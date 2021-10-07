/**
 * @file    http.c
 *
 * @brief   HTTP Source File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-12
 */


#include "http.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"

#include <string.h>


extern const uint8_t influxdb_pem_start[] asm("_binary_influxdb_pem_start");
extern const uint8_t influxdb_pem_end[]   asm("_binary_influxdb_pem_end");

static http_data_en http_data_flag = HTTP_DATA_OK;

static char field[HTTP_FIELD_SIZE];
static uint32_t field_len;


/**
 * @brief     Handles HTTP client events.
 *
 * @param evt The event to be handled.
 *
 * @return  - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  static char *output_buffer;  // Buffer to store response of http request from event handler
  static int output_len;       // Stores number of bytes read
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_HEADERS_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      /*
       *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
       *  However, event handler can also be used in case chunked encoding is used.
       */
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // If user_data buffer is configured, copy the response into the buffer
        if (evt->user_data) {
          memcpy(evt->user_data + output_len, evt->data, evt->data_len);
        } else {
          if (output_buffer == NULL) {
            output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
            output_len = 0;
            if (output_buffer == NULL) {
              ESP_LOGE(HTTP_TAG, "Failed to allocate memory for output buffer");
              return ESP_FAIL;
            }
          }
          memcpy(output_buffer + output_len, evt->data, evt->data_len);
        }
        output_len += evt->data_len;
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
      if (output_buffer != NULL) {
        // Response is accumulated in the output_buffer.
        ESP_LOGD(HTTP_TAG, "%s", output_buffer);
        free(output_buffer);
        output_buffer = NULL;
      }
      output_len = 0;
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
      int mbedtls_err = 0;
      esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
      if (err != 0) {
        if (output_buffer != NULL) {
          free(output_buffer);
          output_buffer = NULL;
        }
        output_len = 0;
        ESP_LOGE(HTTP_TAG, "Last esp error code: 0x%x", err);
        ESP_LOGE(HTTP_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
      }
      break;
  }
  return ESP_OK;
}


/**
 * @brief           The HTTP task function. Checks for pending data and posts it to the InfluxDB.
 */
void http_task()
{
  esp_http_client_config_t http_config = {
    .url = HTTP_POST_URL,
    .cert_pem = (const char*)influxdb_pem_start,
    .skip_cert_common_name_check = true,
    .timeout_ms = HTTP_TIMEOUT_MS,
    .event_handler = http_event_handler
  };

  while (1) {
    if (http_data_flag == HTTP_DATA_PENDING) {
      esp_http_client_handle_t http_client = esp_http_client_init(&http_config);

      esp_http_client_set_method(http_client, HTTP_METHOD_POST);
      esp_http_client_set_header(http_client, "Content-Type", "text/plain");
      esp_http_client_set_post_field(http_client, field, field_len);

      esp_err_t esp_err = esp_http_client_perform(http_client);

      if (esp_err == ESP_OK) {
        ESP_LOGD(HTTP_TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(http_client), esp_http_client_get_content_length(http_client));
      } else {
        ESP_LOGE(HTTP_TAG, "Perform failed with error 0x%x", esp_err);
      }

      esp_err = esp_http_client_cleanup(http_client);
      if (esp_err != ESP_OK) {
        ESP_LOGE(HTTP_TAG, "Cleanup failed with error 0x%x", esp_err);
      }

     http_data_flag = HTTP_DATA_OK;
    } else {
      vTaskDelay(HTTP_POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }
  }


}


/**
 * @brief           Prepares a data send request.
 *
 * @remarks         If HTTP_DATA_PENDING is returned, then the request has to be repeated, because another request is waiting to be served.
 *
 * @param data      The POST field data.
 * @param data_len  The POST field data length.
 *
 * @return        - HTTP_DATA_OK
 *                - HTTP_DATA_PENDING
 */
http_data_en http_send(char *data, uint32_t data_len)
{
  // Checks if other data is waiting to be sent.
  if (http_data_flag == HTTP_DATA_PENDING) {
    return HTTP_DATA_PENDING;
  }

  memcpy(field, data, data_len);
  field_len = data_len;
  http_data_flag = HTTP_DATA_PENDING;

  return HTTP_DATA_OK;
}
