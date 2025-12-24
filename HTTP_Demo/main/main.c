/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_https_server.h>
#include <esp_http_server.h>
#include "esp_tls.h"
#include "sdkconfig.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <string.h>
#include "driver/gpio.h"

#include "oled_integration.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers and start an HTTPS server.
*/

static const char *TAG = "example";
const char *FETCH_URL = "https://api.chucknorris.io/jokes/random";

/* GPIO definitions - adjust these to match your hardware */
#define LED_PIN GPIO_NUM_2  /* GPIO2 - 使用内置LED或连接外部LED */

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} http_buf_t;

static bool led_state = false;  /* 追踪LED状态 */

/* 函数名：http_event_handler
 *
 * 函数说明：HTTP 客户端事件回调，收集响应体到用户缓冲区。
 * 参数：
 *   evt - HTTP 客户端事件指针，user_data 需预先指向 http_buf_t。
 * 返回值：
 *   ESP_OK 表示处理成功。
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_buf_t *ctx = (http_buf_t *) evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (ctx && ctx->buf && ctx->cap > 0) {
                size_t space = ctx->cap - 1 - ctx->len;
                if (space > 0) {
                    size_t copy = evt->data_len < space ? evt->data_len : space;
                    memcpy(ctx->buf + ctx->len, evt->data, copy);
                    ctx->len += copy;
                    ctx->buf[ctx->len] = '\0';
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

/* 函数名：http_fetch_task
 *
 * 函数说明：后台任务，从远端接口抓取笑话并显示到 OLED，失败时显示错误信息。
 * 参数：
 *   pv - 任务参数，未使用。
 * 返回值：
 *   无（任务执行完毕后自删除）。
 */
static void http_fetch_task(void *pv)
{
    http_buf_t ctx = {0};
    ctx.cap = 2048;
    ctx.buf = (char *) calloc(1, ctx.cap);
    if (!ctx.buf) {
        ESP_LOGE(TAG, "no mem for http buffer");
        vTaskDelete(NULL);
        return;
    }

    esp_http_client_config_t cfg = {
        .url = FETCH_URL,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
        .user_data = &ctx,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "http client init failed");
        vTaskDelete(NULL);
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int64_t len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "GET %s => %d, content-length=%lld", FETCH_URL, status, (long long)len);

        if (ctx.len > 0) {
            cJSON *root = cJSON_Parse(ctx.buf);
            if (root) {
                const cJSON *value = cJSON_GetObjectItem(root, "value");
                if (cJSON_IsString(value) && value->valuestring) {
                    ESP_LOGI(TAG, "joke: %s", value->valuestring);
                    oled_show_joke(value->valuestring);
                } else {
                    ESP_LOGW(TAG, "no 'value' field in response");
                    oled_show_error("No value field");
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGW(TAG, "failed to parse JSON");
                oled_show_error("JSON parse error");
            }
        } else {
            ESP_LOGW(TAG, "empty body");
            oled_show_error("Empty response");
        }
    } else {
        ESP_LOGE(TAG, "GET %s failed: %s", FETCH_URL, esp_err_to_name(err));
        oled_show_error("Network error");
    }

    esp_http_client_cleanup(client);
    free(ctx.buf);
    vTaskDelete(NULL);
}

/* Event handler for catching system events */
/* 函数名：event_handler
 *
 * 函数说明：处理 HTTPS 服务器事件，主要记录 TLS 相关错误信息。
 * 参数：
 *   arg - 用户参数，未使用。
 *   event_base - 事件基。
 *   event_id - 事件 ID。
 *   event_data - 事件附带数据。
 * 返回值：
 *   无。
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTPS_SERVER_EVENT) {
        if (event_id == HTTPS_SERVER_EVENT_ERROR) {
            esp_https_server_last_error_t *last_error = (esp_tls_last_error_t *) event_data;
            ESP_LOGE(TAG, "Error event triggered: last_error = %s, last_tls_err = %d, tls_flag = %d", esp_err_to_name(last_error->last_error), last_error->esp_tls_error_code, last_error->esp_tls_flags);
        }
    }
}

/* An HTTP GET handler */
/* 函数名：root_get_handler
 *
 * 函数说明：处理根路径 GET 请求，返回示例 HTML 并添加 CORS 头。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功。
 */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    /* Add CORS headers */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Hello Secure World!</h1>", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

/* OPTIONS handler for CORS preflight */
/* 函数名：options_handler
 *
 * 函数说明：通用 OPTIONS 预检响应，写入 CORS 头并返回 204。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功。
 */
static esp_err_t options_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* LED control handler */
/* 函数名：led_handler
 *
 * 函数说明：处理 /api/led GET，请求参数 action 支持 on/off/toggle 控制板载 LED。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功，错误时返回 400 状态。
 */
static esp_err_t led_handler(httpd_req_t *req)
{
    /* Add CORS headers */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    char query[128];
    char action[16] = {0};
    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "action", action, sizeof(action)) == ESP_OK) {
            if (strcmp(action, "on") == 0) {
                led_state = true;
                gpio_set_level(LED_PIN, 1);
                ESP_LOGI(TAG, "LED ON");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, "{\"status\":\"ok\",\"action\":\"LED ON\"}", HTTPD_RESP_USE_STRLEN);
            } else if (strcmp(action, "off") == 0) {
                led_state = false;
                gpio_set_level(LED_PIN, 0);
                ESP_LOGI(TAG, "LED OFF");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, "{\"status\":\"ok\",\"action\":\"LED OFF\"}", HTTPD_RESP_USE_STRLEN);
            } else if (strcmp(action, "toggle") == 0) {
                led_state = !led_state;
                gpio_set_level(LED_PIN, led_state ? 1 : 0);
                ESP_LOGI(TAG, "LED TOGGLE -> %s", led_state ? "ON" : "OFF");
                httpd_resp_set_type(req, "application/json");
                char response[100];
                snprintf(response, sizeof(response), "{\"status\":\"ok\",\"action\":\"LED %s\"}", led_state ? "ON" : "OFF");
                httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
            } else {
                httpd_resp_set_status(req, "400 Bad Request");
                httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Invalid action\"}", HTTPD_RESP_USE_STRLEN);
            }
            return ESP_OK;
        }
    }
    
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Missing action parameter\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* GPIO control handler */
/* 函数名：gpio_handler
 *
 * 函数说明：处理 /api/gpio GET，按查询参数 pin/level 动态配置指定引脚输出高低电平。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功，缺参时返回 400。
 */
static esp_err_t gpio_handler(httpd_req_t *req)
{
    /* Add CORS headers */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    char query[128];
    char pin_str[8] = {0};
    char level[8] = {0};
    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "pin", pin_str, sizeof(pin_str)) == ESP_OK &&
            httpd_query_key_value(query, "level", level, sizeof(level)) == ESP_OK) {
            
            int pin = atoi(pin_str);
            int level_val = strcmp(level, "high") == 0 ? 1 : 0;
            
            /* Setup GPIO */
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << pin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_down_en = 0,
                .pull_up_en = 0,
                .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&io_conf);
            gpio_set_level(pin, level_val);
            
            ESP_LOGI(TAG, "GPIO%d set to %s", pin, level_val ? "HIGH" : "LOW");
            
            httpd_resp_set_type(req, "application/json");
            char response[100];
            snprintf(response, sizeof(response), "{\"status\":\"ok\",\"gpio\":%d,\"level\":\"%s\"}", pin, level_val ? "HIGH" : "LOW");
            httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
    }
    
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Missing pin or level parameter\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Joke trigger handler */
/* 函数名：joke_handler
 *
 * 函数说明：处理 /api/joke GET，立即响应并启动后台任务抓取笑话。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功。
 */
static esp_err_t joke_handler(httpd_req_t *req)
{
    /* Add CORS headers */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    ESP_LOGI(TAG, "Joke request received - fetching joke...");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Fetching joke...\"}", HTTPD_RESP_USE_STRLEN);
    
    /* Trigger joke fetch in background */
    xTaskCreate(http_fetch_task, "http_fetch", 4096, NULL, 5, NULL);
    
    return ESP_OK;
}

/* OLED text display handler */
/* 函数名：oled_text_handler
 *
 * 函数说明：处理 /api/oled GET，解析查询参数 text 显示自定义文本，或 action=clear 清屏。
 * 参数：
 *   req - HTTP 请求上下文。
 * 返回值：
 *   ESP_OK 表示处理成功，缺参时返回 400。
 */
static esp_err_t oled_text_handler(httpd_req_t *req)
{
    /* Add CORS headers */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    char query[512];
    char text[256] = {0};
    
    /* Get query string */
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        /* Try to get 'text' parameter */
        if (httpd_query_key_value(query, "text", text, sizeof(text)) == ESP_OK) {
            ESP_LOGI(TAG, "Received text for OLED: %s", text);
            
            /* URL decode the text (replace %20 with space, etc.) */
            char decoded[256] = {0};
            size_t decoded_len = 0;
            for (size_t i = 0; i < strlen(text) && decoded_len < sizeof(decoded) - 1; i++) {
                if (text[i] == '%' && i + 2 < strlen(text)) {
                    /* Convert hex to char */
                    char hex[3] = {text[i+1], text[i+2], 0};
                    decoded[decoded_len++] = (char)strtol(hex, NULL, 16);
                    i += 2;
                } else if (text[i] == '+') {
                    decoded[decoded_len++] = ' ';
                } else {
                    decoded[decoded_len++] = text[i];
                }
            }
            decoded[decoded_len] = '\0';
            
            /* Display on OLED */
            oled_show_custom_text(decoded);
            
            /* Send success response */
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Text displayed on OLED\"}", HTTPD_RESP_USE_STRLEN);
        } else {
            /* Check for action parameter (clear) */
            char action[16] = {0};
            if (httpd_query_key_value(query, "action", action, sizeof(action)) == ESP_OK) {
                if (strcmp(action, "clear") == 0) {
                    ESP_LOGI(TAG, "Clearing OLED display");
                    oled_show_status("", "", "");
                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"OLED cleared\"}", HTTPD_RESP_USE_STRLEN);
                    return ESP_OK;
                }
            }
            
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_send(req, "Missing 'text' or 'action' parameter", HTTPD_RESP_USE_STRLEN);
        }
    } else {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "No query string provided", HTTPD_RESP_USE_STRLEN);
    }
    
    return ESP_OK;
}

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
/* 函数名：print_peer_cert_info
 *
 * 函数说明：打印客户端证书信息（仅在 MBEDTLS 配置下生效）。
 * 参数：
 *   ssl - mbedtls SSL 上下文。
 * 返回值：
 *   无。
 */
static void print_peer_cert_info(const mbedtls_ssl_context *ssl)
{
    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    // Logging the peer certificate info
    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
#endif
/**
 * Example callback function to get the certificate of connected clients,
 * whenever a new SSL connection is created and closed
 *
 * Can also be used to other information like Socket FD, Connection state, etc.
 *
 * NOTE: This callback will not be able to obtain the client certificate if the
 * following config `Set minimum Certificate Verification mode to Optional` is
 * not enabled (enabled by default in this example).
 *
 * The config option is found here - Component config → ESP-TLS
 *
 */
/* 函数名：https_server_user_callback
 *
 * 函数说明：HTTPS 用户回调，在会话创建/关闭时记录套接字与证书信息。
 * 参数：
 *   user_cb - HTTPS 服务器回调参数。
 * 返回值：
 *   无。
 */
static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb)
{
    ESP_LOGI(TAG, "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
    mbedtls_ssl_context *ssl_ctx = NULL;
#endif
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGD(TAG, "At session creation");

            // Logging the socket FD
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGD(TAG, "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
#endif
            break;
        default:
            ESP_LOGE(TAG, "Illegal state!");
            return;
    }
}
#endif

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

static const httpd_uri_t root_options = {
    .uri       = "/",
    .method    = HTTP_OPTIONS,
    .handler   = options_handler
};

static const httpd_uri_t oled_text = {
    .uri       = "/api/oled",
    .method    = HTTP_GET,
    .handler   = oled_text_handler
};

static const httpd_uri_t oled_text_options = {
    .uri       = "/api/oled",
    .method    = HTTP_OPTIONS,
    .handler   = options_handler
};

static const httpd_uri_t led_uri = {
    .uri       = "/api/led",
    .method    = HTTP_GET,
    .handler   = led_handler
};

static const httpd_uri_t led_options = {
    .uri       = "/api/led",
    .method    = HTTP_OPTIONS,
    .handler   = options_handler
};

static const httpd_uri_t gpio_uri = {
    .uri       = "/api/gpio",
    .method    = HTTP_GET,
    .handler   = gpio_handler
};

static const httpd_uri_t gpio_options = {
    .uri       = "/api/gpio",
    .method    = HTTP_OPTIONS,
    .handler   = options_handler
};

static const httpd_uri_t joke_uri = {
    .uri       = "/api/joke",
    .method    = HTTP_GET,
    .handler   = joke_handler
};

static const httpd_uri_t joke_options = {
    .uri       = "/api/joke",
    .method    = HTTP_OPTIONS,
    .handler   = options_handler
};

/* Start HTTP server (port 80) */
/* 函数名：start_http_server
 *
 * 函数说明：启动 HTTP 服务器（80 端口），注册所有 URI 处理器。
 * 参数：
 *   无。
 * 返回值：
 *   成功返回服务器句柄，失败返回 NULL。
 */
static httpd_handle_t start_http_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 16;  /* allow enough handlers (root/oled/led/gpio/joke + OPTIONS) */

    ESP_LOGI(TAG, "Starting HTTP server on port 80");
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers for HTTP");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &root_options);
        httpd_register_uri_handler(server, &oled_text);
        httpd_register_uri_handler(server, &oled_text_options);
        httpd_register_uri_handler(server, &led_uri);
        httpd_register_uri_handler(server, &led_options);
        httpd_register_uri_handler(server, &gpio_uri);
        httpd_register_uri_handler(server, &gpio_options);
        httpd_register_uri_handler(server, &joke_uri);
        httpd_register_uri_handler(server, &joke_options);
        return server;
    }

    ESP_LOGE(TAG, "Error starting HTTP server!");
    return NULL;
}

/* Start HTTPS server (port 443) */
/* 函数名：start_https_server
 *
 * 函数说明：启动 HTTPS 服务器（443 端口），加载证书并注册 URI 处理器。
 * 参数：
 *   无。
 * 返回值：
 *   成功返回服务器句柄，失败返回 NULL。
 */
static httpd_handle_t start_https_server(void)
{
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting HTTPS server on port 443");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.max_uri_handlers = 16;  /* mirror HTTP handler capacity */

    extern const unsigned char servercert_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_end[]   asm("_binary_servercert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
    conf.user_cb = https_server_user_callback;
#endif
    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "Error starting HTTPS server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers for HTTPS");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &root_options);
    httpd_register_uri_handler(server, &oled_text);
    httpd_register_uri_handler(server, &oled_text_options);
    httpd_register_uri_handler(server, &led_uri);
    httpd_register_uri_handler(server, &led_options);
    httpd_register_uri_handler(server, &gpio_uri);
    httpd_register_uri_handler(server, &gpio_options);
    httpd_register_uri_handler(server, &joke_uri);
    httpd_register_uri_handler(server, &joke_options);
    return server;
}

/* 函数名：start_webserver
 *
 * 函数说明：同时启动 HTTP 与 HTTPS 服务器，返回优先可用的服务器句柄。
 * 参数：
 *   无。
 * 返回值：
 *   返回 HTTPS 句柄或回退到 HTTP 句柄，均失败时返回 NULL。
 */
static httpd_handle_t start_webserver(void)
{
    /* Start both HTTP and HTTPS servers - return HTTPS handle for compatibility */
    httpd_handle_t http_server = start_http_server();
    httpd_handle_t https_server = start_https_server();
    
    if (!http_server && !https_server) {
        ESP_LOGE(TAG, "Failed to start both HTTP and HTTPS servers!");
        return NULL;
    }
    
    if (http_server) {
        ESP_LOGI(TAG, "✓ HTTP server running on port 80");
    }
    if (https_server) {
        ESP_LOGI(TAG, "✓ HTTPS server running on port 443");
    }
    
    return https_server ? https_server : http_server;
}

/* 函数名：stop_webserver
 *
 * 函数说明：停止 HTTPS 服务器。
 * 参数：
 *   server - 服务器句柄。
 * 返回值：
 *   ESP_OK 表示停止成功。
 */
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_ssl_stop(server);
}

/* 函数名：disconnect_handler
 *
 * 函数说明：网络断开事件回调，尝试停止服务器并清空句柄。
 * 参数：
 *   arg - 指向服务器句柄的指针。
 *   event_base - 事件基。
 *   event_id - 事件 ID。
 *   event_data - 事件数据。
 * 返回值：
 *   无。
 */
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop https server");
        }
    }
}

/* 函数名：connect_handler
 *
 * 函数说明：网络连接事件回调，启动服务器，显示本机 IP，并触发一次笑话抓取任务。
 * 参数：
 *   arg - 指向服务器句柄的指针。
 *   event_base - 事件基。
 *   event_id - 事件 ID。
 *   event_data - 事件数据。
 * 返回值：
 *   无。
 */
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
        
        /* Get and display IP address on OLED */
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "========================================");
                ESP_LOGI(TAG, "ESP32 IP Address: %s", ip_str);
                ESP_LOGI(TAG, "HTTPS Server: https://%s", ip_str);
                ESP_LOGI(TAG, "========================================");
                oled_show_connected_with_ip(ip_str);
            } else {
                oled_show_connected();
            }
        } else {
            oled_show_connected();
        }
    }

    static bool fetch_started = false;
    if (!fetch_started) {
        fetch_started = true;
        xTaskCreate(http_fetch_task, "http_fetch", 4096, NULL, 5, NULL);
    }
}

/* 函数名：app_main
 *
 * 函数说明：应用入口，初始化存储、网络、GPIO、OLED 并注册网络事件回调。
 * 参数：
 *   无。
 * 返回值：
 *   无。
 */
void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize GPIO for LED */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0);  /* LED off initially */
    ESP_LOGI(TAG, "GPIO%d initialized for LED control", LED_PIN);

    /* Initialize OLED display */
    if (oled_init() == ESP_OK) {
        oled_show_connecting();
    }

    /* Register event handlers to start server when Wi-Fi or Ethernet is connected,
     * and stop server when disconnection happens.
     */

#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_SERVER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
}
