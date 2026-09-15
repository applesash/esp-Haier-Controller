#include "board_io.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"

static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_ch422g_control;
static i2c_master_dev_handle_t s_ch422g_output;

static bool s_vsync_callback(esp_lcd_panel_handle_t panel,
                              const esp_lcd_rgb_panel_event_data_t *data,
                              void *context)
{
    (void)panel;
    (void)data;
    (void)context;
    return lvgl_port_notify_rgb_vsync();
}

static esp_err_t board_io_init_i2c(void)
{
    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus), BOARD_IO_TAG,
                        "failed to create I2C bus");

    const i2c_device_config_t control_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BOARD_CH422G_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_i2c_bus, &control_config,
                                                  &s_ch422g_control), BOARD_IO_TAG,
                        "failed to add CH422G control device");

    const i2c_device_config_t output_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BOARD_LCD_AUX_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(s_i2c_bus, &output_config,
                                     &s_ch422g_output);
}

i2c_master_bus_handle_t board_io_i2c_bus(void)
{
    return s_i2c_bus;
}

esp_err_t board_io_backlight_on(void)
{
    const uint8_t output_mode = 0x01;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_ch422g_control, &output_mode, 1,
                                            I2C_MASTER_TIMEOUT_MS), BOARD_IO_TAG,
                        "failed to configure CH422G output mode");

    const uint8_t backlight_on = 0x1e;
    return i2c_master_transmit(s_ch422g_output, &backlight_on, 1,
                               I2C_MASTER_TIMEOUT_MS);
}

esp_err_t board_io_backlight_off(void)
{
    const uint8_t output_mode = 0x01;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_ch422g_control, &output_mode, 1,
                                            I2C_MASTER_TIMEOUT_MS), BOARD_IO_TAG,
                        "failed to configure CH422G output mode");

    const uint8_t backlight_off = 0x1a;
    return i2c_master_transmit(s_ch422g_output, &backlight_off, 1,
                               I2C_MASTER_TIMEOUT_MS);
}

esp_err_t board_io_init(void)
{
    ESP_LOGI(BOARD_IO_TAG, "Initialize 28141 RGB panel");

    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = BOARD_IO_LCD_PIXEL_CLOCK_HZ,
            .h_res = BOARD_IO_LCD_H_RES,
            .v_res = BOARD_IO_LCD_V_RES,
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
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS,
        .bounce_buffer_size_px = BOARD_IO_RGB_BOUNCE_BUFFER_SIZE,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = BOARD_LCD_RGB_HSYNC,
        .vsync_gpio_num = BOARD_LCD_RGB_VSYNC,
        .de_gpio_num = BOARD_LCD_RGB_DE,
        .pclk_gpio_num = BOARD_LCD_RGB_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            BOARD_LCD_RGB_DATA0, BOARD_LCD_RGB_DATA1, BOARD_LCD_RGB_DATA2,
            BOARD_LCD_RGB_DATA3, BOARD_LCD_RGB_DATA4, BOARD_LCD_RGB_DATA5,
            BOARD_LCD_RGB_DATA6, BOARD_LCD_RGB_DATA7, BOARD_LCD_RGB_DATA8,
            BOARD_LCD_RGB_DATA9, BOARD_LCD_RGB_DATA10, BOARD_LCD_RGB_DATA11,
            BOARD_LCD_RGB_DATA12, BOARD_LCD_RGB_DATA13, BOARD_LCD_RGB_DATA14,
            BOARD_LCD_RGB_DATA15,
        },
        .flags.fb_in_psram = 1,
    };

    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_rgb_panel(&panel_config, &panel), BOARD_IO_TAG,
                        "failed to create RGB panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), BOARD_IO_TAG,
                        "failed to initialize RGB panel");
    ESP_RETURN_ON_ERROR(board_io_init_i2c(), BOARD_IO_TAG,
                        "failed to initialize board I2C");
    ESP_RETURN_ON_ERROR(board_io_backlight_on(), BOARD_IO_TAG,
                        "failed to enable backlight");
    ESP_RETURN_ON_ERROR(board_lvgl_init(panel), BOARD_IO_TAG,
                        "failed to initialize LVGL");

    const esp_lcd_rgb_panel_event_callbacks_t callbacks = {
#if BOARD_IO_RGB_BOUNCE_BUFFER_SIZE > 0
        .on_bounce_frame_finish = s_vsync_callback,
#else
        .on_vsync = s_vsync_callback,
#endif
    };
    return esp_lcd_rgb_panel_register_event_callbacks(panel, &callbacks, NULL);
}
