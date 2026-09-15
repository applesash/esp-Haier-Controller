#include <stdint.h>
#include <stdbool.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"

// Waveshare 28141 BOX pinout from the official schematic/documentation.
#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_PCLK_HZ (16 * 1000 * 1000)
#define LCD_PCLK_GPIO 7
#define LCD_HSYNC_GPIO 46
#define LCD_VSYNC_GPIO 3
#define LCD_DE_GPIO 5
#define LCD_DATA0_GPIO 14
#define LCD_DATA1_GPIO 38
#define LCD_DATA2_GPIO 18
#define LCD_DATA3_GPIO 17
#define LCD_DATA4_GPIO 10
#define LCD_DATA5_GPIO 39
#define LCD_DATA6_GPIO 0
#define LCD_DATA7_GPIO 45
#define LCD_DATA8_GPIO 48
#define LCD_DATA9_GPIO 47
#define LCD_DATA10_GPIO 21
#define LCD_DATA11_GPIO 1
#define LCD_DATA12_GPIO 2
#define LCD_DATA13_GPIO 42
#define LCD_DATA14_GPIO 41
#define LCD_DATA15_GPIO 40
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO 8
#define I2C_SCL_GPIO 9
#define CH422G_CONTROL_ADDRESS 0x24
#define CH422G_OUTPUT_ADDRESS 0x38
#define I2C_SPEED_HZ 400000
#define LVGL_BUFFER_HEIGHT CONFIG_TESTSCREEN_LVGL_BUFFER_HEIGHT

static const char *TAG = "testScreen";
static SemaphoreHandle_t s_lvgl_mutex;
static lv_obj_t *s_message;

static esp_err_t init_i2c_and_backlight(void)
{
    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &bus), TAG,
                        "I2C bus creation failed");

    const i2c_device_config_t control_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CH422G_CONTROL_ADDRESS,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    const i2c_device_config_t output_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CH422G_OUTPUT_ADDRESS,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    i2c_master_dev_handle_t control = NULL;
    i2c_master_dev_handle_t output = NULL;
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &control_config, &control),
                        TAG, "CH422G control device creation failed");
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &output_config, &output),
                        TAG, "CH422G output device creation failed");

    const uint8_t output_mode = 0x01;
    const uint8_t backlight_on = 0x1e;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(control, &output_mode, 1, 1000),
                        TAG, "CH422G output mode failed");
    return i2c_master_transmit(output, &backlight_on, 1, 1000);
}

static void flush_cb(lv_disp_drv_t *driver, const lv_area_t *area,
                     lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = driver->user_data;
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1,
                              area->y2 + 1, color_map);
    lv_disp_flush_ready(driver);
}

static void lvgl_tick(void *arg)
{
    (void)arg;
    lv_tick_inc(CONFIG_TESTSCREEN_LVGL_TICK_MS);
}

static esp_err_t init_lvgl(esp_lcd_panel_handle_t panel)
{
    lv_init();
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_args, &tick_timer), TAG,
                        "LVGL timer creation failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(tick_timer,
                                                 CONFIG_TESTSCREEN_LVGL_TICK_MS * 1000),
                        TAG, "LVGL timer start failed");

    const size_t buffer_pixels = LCD_H_RES * LVGL_BUFFER_HEIGHT;
    lv_color_t *buffer = heap_caps_malloc(buffer_pixels * sizeof(lv_color_t),
                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG,
                        "LVGL buffer allocation failed");

    static lv_disp_draw_buf_t draw_buffer;
    static lv_disp_drv_t display_driver;
    lv_disp_draw_buf_init(&draw_buffer, buffer, NULL, buffer_pixels);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = LCD_H_RES;
    display_driver.ver_res = LCD_V_RES;
    display_driver.flush_cb = flush_cb;
    display_driver.draw_buf = &draw_buffer;
    display_driver.user_data = panel;
    ESP_RETURN_ON_FALSE(lv_disp_drv_register(&display_driver) != NULL, ESP_FAIL,
                        TAG, "LVGL display registration failed");

    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    ESP_RETURN_ON_FALSE(s_lvgl_mutex != NULL, ESP_ERR_NO_MEM, TAG,
                        "LVGL mutex creation failed");
    return ESP_OK;
}

static void lvgl_task(void *arg)
{
    (void)arg;
    while (true) {
        if (xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            uint32_t delay_ms = lv_timer_handler();
            xSemaphoreGiveRecursive(s_lvgl_mutex);
            if (delay_ms > 100) {
                delay_ms = 100;
            }
            vTaskDelay(pdMS_TO_TICKS(delay_ms < 1 ? 1 : delay_ms));
        }
    }
}

static void button_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_label_set_text(s_message, "Button pressed");
    }
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x142329), 0);

    s_message = lv_label_create(screen);
    lv_label_set_text(s_message, "Hello, world!");
    lv_obj_set_style_text_color(s_message, lv_color_hex(0xedf4f2), 0);
    lv_obj_set_style_text_font(s_message, &lv_font_montserrat_24, 0);
    lv_obj_align(s_message, LV_ALIGN_CENTER, 0, -45);

    lv_obj_t *button = lv_btn_create(screen);
    lv_obj_set_size(button, 220, 64);
    lv_obj_set_style_radius(button, 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2c6f76), 0);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 45);
    lv_obj_add_event_cb(button, button_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_text = lv_label_create(button);
    lv_label_set_text(button_text, "Press me");
    lv_obj_center(button_text);
}

static esp_err_t init_rgb_panel(esp_lcd_panel_handle_t *panel)
{
    const esp_lcd_rgb_panel_config_t config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PCLK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags.pclk_active_neg = 1,
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = LCD_H_RES * 10,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = LCD_HSYNC_GPIO,
        .vsync_gpio_num = LCD_VSYNC_GPIO,
        .de_gpio_num = LCD_DE_GPIO,
        .pclk_gpio_num = LCD_PCLK_GPIO,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            LCD_DATA0_GPIO, LCD_DATA1_GPIO, LCD_DATA2_GPIO, LCD_DATA3_GPIO,
            LCD_DATA4_GPIO, LCD_DATA5_GPIO, LCD_DATA6_GPIO, LCD_DATA7_GPIO,
            LCD_DATA8_GPIO, LCD_DATA9_GPIO, LCD_DATA10_GPIO, LCD_DATA11_GPIO,
            LCD_DATA12_GPIO, LCD_DATA13_GPIO, LCD_DATA14_GPIO, LCD_DATA15_GPIO,
        },
        .flags.fb_in_psram = 1,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&config, panel), TAG,
                        "RGB panel creation failed");
    return esp_lcd_panel_init(*panel);
}

void app_main(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(init_rgb_panel(&panel));
    ESP_ERROR_CHECK(init_i2c_and_backlight());
    ESP_ERROR_CHECK(init_lvgl(panel));
    if (xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        build_ui();
        lv_obj_invalidate(lv_scr_act());
        lv_refr_now(lv_disp_get_default());
        xSemaphoreGiveRecursive(s_lvgl_mutex);
    }
    ESP_ERROR_CHECK(xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL,
                                            CONFIG_TESTSCREEN_LVGL_TASK_PRIORITY,
                                            NULL, CONFIG_TESTSCREEN_LVGL_TASK_CORE) == pdPASS
                        ? ESP_OK : ESP_FAIL);
    ESP_LOGI(TAG, "testScreen ready: Hello World and one button");
}
