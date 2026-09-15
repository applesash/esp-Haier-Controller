#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_28141.h"
#include "board_io.h"
#include "heat_ch422g.h"
#include "heat_observer.h"
#include "heat_web.h"

static const char *HAIER_TAG = "haier_controller";
static lv_obj_t *s_overview_screen;
static lv_obj_t *s_hello_label;

static void show_settings(lv_event_t *event);
static void show_overview(lv_event_t *event);

static void hello_button_event(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED && s_hello_label != NULL) {
        lv_label_set_text(s_hello_label, "Hello, world! Button pressed");
    }
}

static void build_hello_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_scr_load(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x142329), 0);

    s_hello_label = lv_label_create(screen);
    lv_label_set_text(s_hello_label, "Hello, world!");
    lv_obj_set_style_text_color(s_hello_label, lv_color_hex(0xedf4f2), 0);
    lv_obj_align(s_hello_label, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *button = lv_btn_create(screen);
    lv_obj_set_size(button, 220, 64);
    lv_obj_set_style_radius(button, 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2c6f76), 0);
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 45);
    lv_obj_add_event_cb(button, hello_button_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Press me");
    lv_obj_center(button_label);
}

static lv_obj_t *settings_row(lv_obj_t *parent, const char *title, const char *summary)
{
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, 740, 54);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1b3037), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x294047), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text_fmt(label, "%s\n%s", title, summary);
    lv_obj_set_style_text_color(label, lv_color_hex(0xedf4f2), 0);
    lv_obj_center(label);
    return row;
}

static void add_tile(lv_obj_t *grid, const char *label, const char *value,
                     const char *unit, const char *detail, uint32_t accent)
{
    uint32_t tile_index = lv_obj_get_child_cnt(grid);
    lv_obj_t *tile = lv_obj_create(grid);
    lv_obj_set_size(tile, 250, 110);
    lv_obj_set_grid_cell(tile, LV_GRID_ALIGN_STRETCH, tile_index % 3, 1,
                         LV_GRID_ALIGN_STRETCH, tile_index / 3, 1);
    lv_obj_set_style_bg_color(tile, lv_color_hex(0x1b3037), 0);
    lv_obj_set_style_border_color(tile, lv_color_hex(0x294047), 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_radius(tile, 0, 0);
    lv_obj_set_style_pad_all(tile, 14, 0);

    lv_obj_t *title = lv_label_create(tile);
    lv_label_set_text(title, label);
    lv_obj_set_style_text_color(title, lv_color_hex(0xa8c0bf), 0);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *reading = lv_label_create(tile);
    lv_label_set_text(reading, value);
    lv_obj_set_style_text_color(reading, lv_color_hex(0xf3f8f4), 0);
    lv_obj_set_style_text_font(reading, LV_FONT_DEFAULT, 0);
    lv_obj_align(reading, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *reading_unit = lv_label_create(tile);
    lv_label_set_text(reading_unit, unit);
    lv_obj_set_style_text_color(reading_unit, lv_color_hex(accent), 0);
    lv_obj_align_to(reading_unit, reading, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, 0);

    lv_obj_t *description = lv_label_create(tile);
    lv_label_set_text(description, detail);
    lv_obj_set_style_text_color(description, lv_color_hex(0x71898d), 0);
    lv_obj_set_style_text_font(description, LV_FONT_DEFAULT, 0);
    lv_obj_align(description, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

static void build_hmi_overview(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_size(screen, BOARD_LVGL_WIDTH, BOARD_LVGL_HEIGHT);
    s_overview_screen = screen;
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x142329), 0);

    lv_obj_t *eyebrow = lv_label_create(screen);
    lv_label_set_text(eyebrow, "HAIER CONTROLLER");
    lv_obj_set_style_text_color(eyebrow, lv_color_hex(0x72d6cf), 0);
    lv_obj_set_pos(eyebrow, 18, 10);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "System overview");
    lv_obj_set_style_text_color(title, lv_color_hex(0xedf4f2), 0);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
    lv_obj_set_pos(title, 18, 28);

    lv_obj_t *status = lv_label_create(screen);
    lv_label_set_text(status, "Passive monitor  |  RS485 listening");
    lv_obj_set_style_text_color(status, lv_color_hex(0x9ab1b3), 0);
    lv_obj_set_pos(status, 500, 54);

    lv_obj_t *settings = lv_btn_create(screen);
    lv_obj_set_size(settings, 110, 38);
    lv_obj_set_style_radius(settings, 0, 0);
    lv_obj_set_style_bg_color(settings, lv_color_hex(0x203a40), 0);
    lv_obj_set_pos(settings, 670, 12);
    lv_obj_add_event_cb(settings, show_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_t *settings_label = lv_label_create(settings);
    lv_label_set_text(settings_label, "Settings");
    lv_obj_center(settings_label);

    lv_obj_t *grid = lv_obj_create(screen);
    lv_obj_set_size(grid, 782, 394);
    lv_obj_set_pos(grid, 9, 78);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static const lv_coord_t columns[] = {250, 250, 250, LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t rows[] = {125, 125, 125, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, columns, rows);

    add_tile(grid, "Outdoor", "8.4", "C", "82.1% RH  fresh 4 s", 0x77b9df);
    add_tile(grid, "Upstairs", "21.6 / 21.0", "C", "48.3% RH  XY-MD02", 0x77b9df);
    add_tile(grid, "Downstairs", "20.9 / 21.0", "C", "51.7% RH  XY-MD02", 0x77b9df);
    add_tile(grid, "DHW tank", "48.2 / 55", "C", "sensor source pending", 0xf0b45d);
    add_tile(grid, "Flow", "42.7 / 45", "C", "heating circuit", 0x72d6cf);
    add_tile(grid, "Return", "34.1", "C", "heating circuit", 0x72d6cf);
    add_tile(grid, "Flow rate", "8.6", "L/min", "calculation pending", 0x72d6cf);
    add_tile(grid, "Thermal output", "5.4", "kW out", "derived from flow delta", 0xf0b45d);
    add_tile(grid, "Thermal input", "6.1", "kW in", "Roomstat data pending", 0xf0b45d);
}

static void show_overview(lv_event_t *event)
{
    (void)event;
    if (s_overview_screen != NULL) {
        lv_scr_load(s_overview_screen);
    }
}

static void show_settings(lv_event_t *event)
{
    (void)event;
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x142329), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Controller settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0xedf4f2), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 18, 14);

    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_set_size(back, 110, 38);
    lv_obj_set_style_radius(back, 0, 0);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x203a40), 0);
    lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -18, 12);
    lv_obj_add_event_cb(back, show_overview, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, "Overview");
    lv_obj_center(back_label);

    lv_obj_t *list = lv_obj_create(screen);
    lv_obj_set_size(list, 760, 360);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    settings_row(list, "Wi-Fi Settings", "Scan networks and connect to a local network");
    settings_row(list, "OTA Updates", "Stable channel · v0.1.0-dev");
    settings_row(list, "RS485 Sensors", "Commission upstairs, downstairs, and outdoor sensors");
    settings_row(list, "Display and Touch", "800 x 480 · touch status");
    settings_row(list, "Field Outputs", "Disabled until explicitly enabled");
    lv_scr_load(screen);
}

void app_main(void)
{
    heat_ch422g_t io_expander = {0};
    uint8_t input_state = 0;

    ESP_LOGI(HAIER_TAG, "Haier controller starting on %s", BOARD_MODEL_NAME);
    ESP_LOGI(HAIER_TAG, "Initial mode: passive RS485 observation");
    ESP_LOGI(HAIER_TAG, "RS485 UART pins: TX=%d RX=%d, automatic direction",
             BOARD_RS485_TX, BOARD_RS485_RX);

    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        error = nvs_flash_erase();
        if (error == ESP_OK) {
            error = nvs_flash_init();
        }
    }
    if (error == ESP_OK) {
        error = esp_netif_init();
    }
    if (error == ESP_OK) {
        error = esp_event_loop_create_default();
    }
    if (error == ESP_OK) {
        const esp_vfs_spiffs_conf_t storage_config = {
            .base_path = "/storage",
            .partition_label = "storage",
            .max_files = 8,
            .format_if_mount_failed = false,
        };
        error = esp_vfs_spiffs_register(&storage_config);
    }
    if (error == ESP_OK) {
        error = heat_web_start();
    }
    if (error != ESP_OK) {
        ESP_LOGW(HAIER_TAG, "Web HMI unavailable: %s", esp_err_to_name(error));
    }
    const heat_modbus_config_t observer_modbus_config = {
        .uart_port = UART_NUM_1,
        .tx_gpio = BOARD_RS485_TX,
        .rx_gpio = BOARD_RS485_RX,
        .baud_rate = 9600,
        .response_timeout_ms = 400,
    };
    heat_web_set_modbus_config(&observer_modbus_config);

    const heat_observer_config_t observer_config = {
        .uart_port = observer_modbus_config.uart_port,
        .tx_gpio = observer_modbus_config.tx_gpio,
        .rx_gpio = observer_modbus_config.rx_gpio,
        .baud_rate = observer_modbus_config.baud_rate,
    };
    error = heat_observer_start(&observer_config);
    if (error != ESP_OK) {
        ESP_LOGW(HAIER_TAG, "Passive RS485 observer unavailable: %s", esp_err_to_name(error));
    }

    if (error == ESP_OK) {
        error = board_io_init();
    }
    ESP_LOGI(HAIER_TAG, "RGB/LVGL startup result: %s", esp_err_to_name(error));
    if (error == ESP_OK) {
        const bool lvgl_locked = lvgl_port_lock(-1);
        ESP_LOGI(HAIER_TAG, "LVGL UI lock: %s", lvgl_locked ? "ok" : "failed");
        if (lvgl_locked) {
            build_hello_screen();
            lv_obj_invalidate(lv_scr_act());
            lv_refr_now(lv_disp_get_default());
            lv_timer_handler();
            lvgl_port_unlock();
        }
    }
    const heat_ch422g_config_t expander_config = {
        .bus = board_io_i2c_bus(),
        .address = HEAT_CH422G_DEFAULT_ADDRESS,
        .scl_speed_hz = HEAT_CH422G_DEFAULT_SCL_SPEED_HZ,
    };
    if (error == ESP_OK) {
        error = heat_ch422g_init(&io_expander, &expander_config);
    }
    if (error == ESP_OK) {
        error = heat_ch422g_read_inputs(&io_expander, &input_state);
    }
    if (error == ESP_OK) {
        ESP_LOGI(HAIER_TAG, "CH422G detected; terminal inputs=0x%02x", input_state);
    } else {
        ESP_LOGW(HAIER_TAG, "CH422G read-only probe unavailable: %s", esp_err_to_name(error));
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
