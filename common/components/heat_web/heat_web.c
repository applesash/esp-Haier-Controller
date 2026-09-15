#include "heat_web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "driver/uart.h"
#include "heat_observer.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "heat_web";
static httpd_handle_t s_server;
static bool s_scan_busy;
static wifi_ap_record_t s_scan_records[16];
static size_t s_scan_count;
static heat_modbus_config_t s_modbus_config;
static bool s_modbus_configured;

void heat_web_set_modbus_config(const heat_modbus_config_t *config)
{
    if (config != NULL) {
        s_modbus_config = *config;
        s_modbus_configured = true;
    }
}

static void wifi_scan_done_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_base;
    (void)event_id;
    (void)event_data;
    uint16_t count = 16;
    if (esp_wifi_scan_get_ap_records(&count, s_scan_records) == ESP_OK) {
        s_scan_count = count;
    } else {
        s_scan_count = 0;
    }
    s_scan_busy = false;
    ESP_LOGI(TAG, "Wi-Fi scan complete: %u networks", (unsigned)s_scan_count);
}

static esp_err_t read_request_body(httpd_req_t *request, char *buffer, size_t size)
{
    if (request->content_len <= 0 || (size_t)request->content_len >= size) {
        return ESP_ERR_INVALID_SIZE;
    }
    int received = httpd_req_recv(request, buffer, request->content_len);
    if (received != request->content_len) {
        return ESP_FAIL;
    }
    buffer[received] = '\0';
    return ESP_OK;
}

static esp_err_t send_file(httpd_req_t *request, const char *path, const char *type)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }
    httpd_resp_set_type(request, type);
    char buffer[1024];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(request, buffer, count) != ESP_OK) {
            fclose(file);
            return ESP_FAIL;
        }
    }
    fclose(file);
    return httpd_resp_send_chunk(request, NULL, 0);
}

static esp_err_t root_handler(httpd_req_t *request)
{
    return send_file(request, "/storage/index.html", "text/html");
}

static esp_err_t asset_handler(httpd_req_t *request)
{
    const char *path = "/storage/index.html";
    const char *type = "text/plain";
    if (strcmp(request->uri, "/app.js") == 0) {
        path = "/storage/app.js";
        type = "application/javascript";
    } else if (strcmp(request->uri, "/style.css") == 0) {
        path = "/storage/style.css";
        type = "text/css";
    } else if (strcmp(request->uri, "/dashboard_model.json") == 0) {
        path = "/storage/dashboard_model.json";
        type = "application/json";
    } else {
        httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }
    return send_file(request, path, type);
}

static esp_err_t status_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request,
        "{\"schema\":1,\"status\":{\"mode\":\"passive-monitor\",\"bus\":\"RS485 listening\",\"discovery\":\"pending\"},"
        "\"wifi\":{\"mode\":\"access-point\",\"ssid\":\"haier-hmi\",\"ip\":\"192.168.4.1\"},"
        "\"commissioning\":{\"stage\":\"read-only\",\"writesEnabled\":false},"
        "\"ota\":{\"status\":\"not checked\",\"writesEnabled\":false}}");
}

static esp_err_t wifi_scan_handler(httpd_req_t *request)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "busy", s_scan_busy);
    cJSON *networks = cJSON_AddArrayToObject(root, "networks");
    for (size_t index = 0; index < s_scan_count; ++index) {
        cJSON *network = cJSON_CreateObject();
        cJSON_AddStringToObject(network, "ssid", (const char *)s_scan_records[index].ssid);
        cJSON_AddNumberToObject(network, "rssi", s_scan_records[index].rssi);
        cJSON_AddItemToArray(networks, network);
    }
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (body == NULL) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON error");
    }
    httpd_resp_set_type(request, "application/json");
    esp_err_t error = httpd_resp_sendstr(request, body);
    free(body);
    return error;
}

static esp_err_t wifi_scan_start_handler(httpd_req_t *request)
{
    if (!s_scan_busy) {
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG,
                            "failed to enable station scan mode");
        const wifi_scan_config_t config = {
            .show_hidden = false,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time.active = {.min = 120, .max = 300},
        };
        ESP_RETURN_ON_ERROR(esp_wifi_scan_start((wifi_scan_config_t *)&config, false), TAG,
                            "Wi-Fi scan start failed");
        s_scan_busy = true;
        s_scan_count = 0;
    }
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request, "{\"started\":true}");
}

static esp_err_t wifi_connect_handler(httpd_req_t *request)
{
    char body[512];
    if (read_request_body(request, body, sizeof(body)) != ESP_OK) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "Expected JSON with ssid and password");
    }
    cJSON *root = cJSON_Parse(body);
    const cJSON *ssid = root == NULL ? NULL : cJSON_GetObjectItem(root, "ssid");
    const cJSON *password = root == NULL ? NULL : cJSON_GetObjectItem(root, "password");
    if (!cJSON_IsString(ssid) || !cJSON_IsString(password) ||
        ssid->valuestring == NULL || password->valuestring == NULL ||
        strlen(ssid->valuestring) == 0 || strlen(ssid->valuestring) > 32 ||
        strlen(password->valuestring) > 64) {
        cJSON_Delete(root);
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "Invalid Wi-Fi credentials");
    }

    nvs_handle_t storage = 0;
    esp_err_t error = nvs_open("haier_wifi", NVS_READWRITE, &storage);
    if (error == ESP_OK) {
        error = nvs_set_str(storage, "ssid", ssid->valuestring);
    }
    if (error == ESP_OK) {
        error = nvs_set_str(storage, "password", password->valuestring);
    }
    if (error == ESP_OK) {
        error = nvs_commit(storage);
    }
    if (storage != 0) {
        nvs_close(storage);
    }
    if (error == ESP_OK) {
        wifi_config_t station_config = {0};
        snprintf((char *)station_config.sta.ssid, sizeof(station_config.sta.ssid),
                 "%s", ssid->valuestring);
        snprintf((char *)station_config.sta.password, sizeof(station_config.sta.password),
                 "%s", password->valuestring);
        error = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (error == ESP_OK) {
            error = esp_wifi_set_config(WIFI_IF_STA, &station_config);
        }
        if (error == ESP_OK) {
            error = esp_wifi_connect();
        }
    }
    cJSON_Delete(root);
    if (error != ESP_OK) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Wi-Fi connection start failed");
    }
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request,
                              "{\"status\":\"connecting\",\"accessPoint\":\"haier-hmi\"}");
}

static esp_err_t commissioning_handler(httpd_req_t *request)
{
    char body[512];
    if (!s_modbus_configured || read_request_body(request, body, sizeof(body)) != ESP_OK) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid commissioning request");
    }
    cJSON *root = cJSON_Parse(body);
    const cJSON *step = root == NULL ? NULL : cJSON_GetObjectItem(root, "step");
    cJSON *response = cJSON_CreateObject();
    bool ok = false;
    const char *message = "Invalid step";
    uint8_t address = root && cJSON_IsNumber(cJSON_GetObjectItem(root, "address"))
                          ? (uint8_t)cJSON_GetObjectItem(root, "address")->valuedouble : 0;
    uint8_t target = root && cJSON_IsNumber(cJSON_GetObjectItem(root, "target_address"))
                         ? (uint8_t)cJSON_GetObjectItem(root, "target_address")->valuedouble : 0;
    heat_observer_stop();
    heat_modbus_master_t master;
    esp_err_t error = heat_modbus_master_init(&master, &s_modbus_config);
    uint16_t registers[2] = {0};
    if (error == ESP_OK && cJSON_IsString(step) && strcmp(step->valuestring, "detect") == 0) {
        error = heat_modbus_read_input_registers(&master, address, 1, 2, registers);
        ok = error == ESP_OK;
        message = ok ? "Sensor detected" : "No sensor at address";
    } else if (error == ESP_OK && cJSON_IsString(step) && strcmp(step->valuestring, "change") == 0) {
        error = heat_modbus_read_input_registers(&master, target, 1, 2, registers);
        if (target != address && error == ESP_OK) {
            message = "Target address already in use";
        } else {
            error = heat_modbus_write_single_register(&master, address, 0x0101, target);
            ok = error == ESP_OK;
            message = ok ? "Address stored; reboot sensor now" : "Address change failed";
        }
    } else if (error == ESP_OK && cJSON_IsString(step) && strcmp(step->valuestring, "verify") == 0) {
        error = heat_modbus_read_input_registers(&master, target, 1, 2, registers);
        ok = error == ESP_OK;
        message = ok ? "New address verified" : "New address did not respond";
    } else if (error == ESP_OK && cJSON_IsString(step) && strcmp(step->valuestring, "calibrate") == 0 &&
               cJSON_IsNumber(cJSON_GetObjectItem(root, "temperature_offset")) &&
               cJSON_IsNumber(cJSON_GetObjectItem(root, "humidity_offset"))) {
        int16_t temperature = (int16_t)(cJSON_GetObjectItem(root, "temperature_offset")->valuedouble * 10.0);
        int16_t humidity = (int16_t)(cJSON_GetObjectItem(root, "humidity_offset")->valuedouble * 10.0);
        error = heat_modbus_write_single_register(&master, target, 0x0103, (uint16_t)temperature);
        if (error == ESP_OK) error = heat_modbus_write_single_register(&master, target, 0x0104, (uint16_t)humidity);
        ok = error == ESP_OK;
        message = ok ? "Calibration written" : "Calibration failed";
    }
    uart_driver_delete(s_modbus_config.uart_port);
    const heat_observer_config_t observer_config = {
        .uart_port = s_modbus_config.uart_port,
        .tx_gpio = s_modbus_config.tx_gpio,
        .rx_gpio = s_modbus_config.rx_gpio,
        .baud_rate = s_modbus_config.baud_rate,
    };
    heat_observer_start(&observer_config);
    cJSON_AddBoolToObject(response, "ok", ok);
    cJSON_AddStringToObject(response, "message", message);
    cJSON_AddNumberToObject(response, "temperature", (int16_t)registers[0] / 10.0);
    cJSON_AddNumberToObject(response, "humidity", registers[1] / 10.0);
    char *output = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    cJSON_Delete(root);
    httpd_resp_set_type(request, "application/json");
    esp_err_t send_error = httpd_resp_sendstr(request, output != NULL ? output : "{\"ok\":false}");
    free(output);
    return send_error;
}

static esp_err_t ota_check_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_sendstr(request,
                              "{\"status\":\"not checked\",\"updateAvailable\":false,\"writesEnabled\":false}");
}

static esp_err_t ota_apply_handler(httpd_req_t *request)
{
    return httpd_resp_send_err(request, HTTPD_403_FORBIDDEN,
                               "OTA installation is locked until HTTPS validation and rollback are implemented");
}

esp_err_t heat_web_start(void)
{
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "Wi-Fi init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                                    wifi_scan_done_handler, NULL), TAG,
                        "Wi-Fi scan event registration failed");
    wifi_config_t ap_config = {
        .ap = {
            .ssid = HEAT_WEB_AP_SSID,
            .ssid_len = sizeof(HEAT_WEB_AP_SSID) - 1,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    wifi_config_t station_config = {0};
    bool has_station_config = false;
    nvs_handle_t storage = 0;
    if (nvs_open("haier_wifi", NVS_READONLY, &storage) == ESP_OK) {
        size_t ssid_size = sizeof(station_config.sta.ssid);
        size_t password_size = sizeof(station_config.sta.password);
        has_station_config = nvs_get_str(storage, "ssid",
                                         (char *)station_config.sta.ssid,
                                         &ssid_size) == ESP_OK &&
                             nvs_get_str(storage, "password",
                                         (char *)station_config.sta.password,
                                         &password_size) == ESP_OK;
        nvs_close(storage);
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(has_station_config ? WIFI_MODE_APSTA : WIFI_MODE_AP),
                        TAG, "Wi-Fi mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_config), TAG,
                        "Wi-Fi AP config failed");
    if (has_station_config) {
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &station_config), TAG,
                            "Wi-Fi station config failed");
    }
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Wi-Fi start failed");
    if (has_station_config) {
        ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "Wi-Fi station connect failed");
    }
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.server_port = HEAT_WEB_HTTP_PORT;
    server_config.max_uri_handlers = 16;
    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &server_config), TAG,
                        "HTTP server start failed");
    const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = root_handler},
        {.uri = "/app.js", .method = HTTP_GET, .handler = asset_handler},
        {.uri = "/style.css", .method = HTTP_GET, .handler = asset_handler},
        {.uri = "/dashboard_model.json", .method = HTTP_GET, .handler = asset_handler},
        {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/wifi/scan", .method = HTTP_GET, .handler = wifi_scan_handler},
        {.uri = "/api/wifi/scan", .method = HTTP_POST, .handler = wifi_scan_start_handler},
        {.uri = "/api/wifi/connect", .method = HTTP_POST, .handler = wifi_connect_handler},
        {.uri = "/api/modbus/commission", .method = HTTP_POST, .handler = commissioning_handler},
        {.uri = "/api/ota/check", .method = HTTP_GET, .handler = ota_check_handler},
        {.uri = "/api/ota/apply", .method = HTTP_POST, .handler = ota_apply_handler},
    };
    for (size_t index = 0; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &routes[index]), TAG,
                            "HTTP route registration failed");
    }
    ESP_LOGI(TAG, "Web HMI available at http://192.168.4.1/");
    return ESP_OK;
}
