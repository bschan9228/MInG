#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"

#include <esp_http_server.h>

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdlib.h>
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"

#include <cJSON.h>

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");
extern const char root_js_start[] asm("_binary_root_js_start");
extern const char root_js_end[] asm("_binary_root_js_end");

extern const char keyboard_start[] asm("_binary_keyboard_html_start");
extern const char keyboard_end[] asm("_binary_keyboard_html_end");
extern const char keyboard_js_start[] asm("_binary_keyboard_js_start");
extern const char keyboard_js_end[] asm("_binary_keyboard_js_end");

extern const char pm_start[] asm("_binary_pm_html_start");
extern const char pm_end[] asm("_binary_pm_html_end");
extern const char pm_js_start[] asm("_binary_pm_js_start");
extern const char pm_js_end[] asm("_binary_pm_js_end");

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "console";

//------------------------- USB HID -------------------------//

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

/********* Application ***************/

typedef enum
{
    MOUSE_DIR_RIGHT,
    MOUSE_DIR_DOWN,
    MOUSE_DIR_LEFT,
    MOUSE_DIR_UP,
    MOUSE_DIR_MAX,
} mouse_dir_t;

#define DISTANCE_MAX 125
#define DELTA_SCALAR 5

static void mouse_draw_square_next_delta(int8_t *delta_x_ret, int8_t *delta_y_ret)
{
    static mouse_dir_t cur_dir = MOUSE_DIR_RIGHT;
    static uint32_t distance = 0;

    // Calculate next delta
    if (cur_dir == MOUSE_DIR_RIGHT)
    {
        *delta_x_ret = DELTA_SCALAR;
        *delta_y_ret = 0;
    }
    else if (cur_dir == MOUSE_DIR_DOWN)
    {
        *delta_x_ret = 0;
        *delta_y_ret = DELTA_SCALAR;
    }
    else if (cur_dir == MOUSE_DIR_LEFT)
    {
        *delta_x_ret = -DELTA_SCALAR;
        *delta_y_ret = 0;
    }
    else if (cur_dir == MOUSE_DIR_UP)
    {
        *delta_x_ret = 0;
        *delta_y_ret = -DELTA_SCALAR;
    }

    // Update cumulative distance for current direction
    distance += DELTA_SCALAR;
    // Check if we need to change direction
    if (distance >= DISTANCE_MAX)
    {
        distance = 0;
        cur_dir++;
        if (cur_dir == MOUSE_DIR_MAX)
        {
            cur_dir = 0;
        }
    }
}

static void app_send_hid_demo(void)
{
    // Keyboard output: Send key 'a/A' pressed and released
    ESP_LOGI(TAG, "Sending Keyboard report");
    uint8_t keycode[6] = {HID_KEY_A};
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);

    // Mouse output: Move mouse cursor in square trajectory
    ESP_LOGI(TAG, "Sending Mouse report");
    int8_t delta_x;
    int8_t delta_y;
    for (int i = 0; i < (DISTANCE_MAX / DELTA_SCALAR) * 4; i++)
    {
        // Get the next x and y delta in the draw square pattern
        mouse_draw_square_next_delta(&delta_x, &delta_y);
        tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, 0x00, delta_x, delta_y, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

uint8_t const conv_table[128][2] = {HID_ASCII_TO_KEYCODE};

static void send_character(char c)
{
    uint8_t keycode[6] = {0};
    uint8_t modifier = 0;
    if (conv_table[c + 0][0])
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
    keycode[0] = conv_table[c + 0][1];

    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
    // Can speed up by removing null report and adding more keycodes
    vTaskDelay(pdMS_TO_TICKS(10));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void app_send_string(uint8_t *data, int start, size_t len)
{
    // Keyboard sends inputted string
    ESP_LOGI(TAG, "Sending Keyboard report");
    for (int i = start; i < len; i++)
    {
        send_character(data[i]);
    }
}

// --------------------------- Websockets --------------------------- //
/* A simple example that demonstrates using websocket echo server
 */

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg
{
    httpd_handle_t hd;
    int fd;
};

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg)
{
    static const char *data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len)
    {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);

        // ---------------------------------------- Sends an HID when a packet a ws packet is recieved ---------------------------------------- //
        // if (tud_mounted())
        // {
        //     app_send_string(ws_pkt.payload, ws_pkt.len);
        // }
        // ----- Sends an HID depending on JSON packet ----- //
        cJSON *web_json = cJSON_ParseWithLength((char *)ws_pkt.payload, ws_pkt.len);
        cJSON *app = cJSON_GetObjectItemCaseSensitive(web_json, "app");
        ESP_LOGI("App payload", "[%s]", cJSON_Print(app));

        // if (strcmp(cJSON_Print(app), "\"pm\"") == 0)
        if (tud_mounted() && strcmp(cJSON_Print(app), "\"pm\"") == 0)
        {
            cJSON *data = cJSON_GetObjectItemCaseSensitive(web_json, "data");
            ESP_LOGI("Data payload", "[%s]", cJSON_Print(data));
            app_send_string((uint8_t *)cJSON_Print(data), 1, strlen(cJSON_Print(data)) - 1); // use this
            // app_send_string(ws_pkt.payload, ws_pkt.len);
        }

        else if (tud_mounted() && (strcmp(cJSON_Print(app), "\"keyboard\"") == 0 || strcmp(cJSON_Print(app), "\"root\"") == 0))
        {
            cJSON *data = cJSON_GetObjectItemCaseSensitive(web_json, "data");
            app_send_string((uint8_t *)cJSON_Print(data), 1, strlen(cJSON_Print(data)) - 1); // use this
        }
        // ESP_LOGI("WebSocket: ", "Characters in packet: %s\n Length: %i", cJSON_Print(type), strlen(cJSON_Print(type)) - 2);
        // ESP_LOGI(TAG, "First character: %c", ws_pkt.payload[0]);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);

    // Sends response back to ws
    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}
// ------------------------------------- Websockets ------------------------------------- //
static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = echo_handler,
    .user_ctx = NULL,
    .is_websocket = true};

// ------------------------------------- Root html ------------------------------------- //
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// ------------------------------------- Root javascript ------------------------------------- //
static esp_err_t root_js_get_handler(httpd_req_t *req)
{
    const uint32_t root_js_len = root_js_end - root_js_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_js_start, root_js_len);

    return ESP_OK;
}

static const httpd_uri_t root_js = {
    .uri = "/root.js",
    .method = HTTP_GET,
    .handler = root_js_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// ------------------------------------- Keyboard html ------------------------------------- //
static esp_err_t keyboard_get_handler(httpd_req_t *req)
{
    const uint32_t keyboard_len = keyboard_end - keyboard_start;

    ESP_LOGI(TAG, "Serve keyboard");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, keyboard_start, keyboard_len);

    return ESP_OK;
}

static const httpd_uri_t keyboard = {
    .uri = "/keyboard",
    .method = HTTP_GET,
    .handler = keyboard_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// ------------------------------------- Keyboard javascript ------------------------------------- //
static esp_err_t keyboard_js_get_handler(httpd_req_t *req)
{
    const uint32_t keyboard_js_len = keyboard_js_end - keyboard_js_start;

    ESP_LOGI(TAG, "Serve keyboard");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, keyboard_js_start, keyboard_js_len);

    return ESP_OK;
}

static const httpd_uri_t keyboard_js = {
    .uri = "/keyboard.js",
    .method = HTTP_GET,
    .handler = keyboard_js_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// ------------------------------------- Password manager html ------------------------------------- //
static esp_err_t pm_get_handler(httpd_req_t *req)
{
    const uint32_t pm_len = pm_end - pm_start;

    ESP_LOGI(TAG, "Serve password manager");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, pm_start, pm_len);

    return ESP_OK;
}

static const httpd_uri_t pm = {
    .uri = "/pm",
    .method = HTTP_GET,
    .handler = pm_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// ------------------------------------- Password manager javascript ------------------------------------- //
static esp_err_t pm_js_get_handler(httpd_req_t *req)
{
    const uint32_t pm_js_len = pm_js_end - pm_js_start;

    ESP_LOGI(TAG, "Serve Password manager");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, pm_js_start, pm_js_len);

    return ESP_OK;
}

static const httpd_uri_t pm_js = {
    .uri = "/pm.js",
    .method = HTTP_GET,
    .handler = pm_js_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

// -------------------------------------  ------------------------------------- //

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {

        ESP_LOGI(TAG, "Registering URI handlers");
        // Registering the ws handler
        httpd_register_uri_handler(server, &ws);
        // Registering root handlers
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &root_js);
        // Registering keyboard handlers
        httpd_register_uri_handler(server, &keyboard);
        httpd_register_uri_handler(server, &keyboard_js);
        // Registering password manager
        httpd_register_uri_handler(server, &pm);
        httpd_register_uri_handler(server, &pm_js);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

// Soft AP
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t wifi_init_softap(void)
{
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    return ESP_OK;
}

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    ESP_ERROR_CHECK(wifi_init_softap());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    server = start_webserver();

    // Tiny USB
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
}

/*
Testing:
ip = "192.168.4.1" //?
ws = new WebSocket('ws://' + ip + '/ws');
ws.send("abc");

*/