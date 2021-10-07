/**
 * @file    wifi.c
 *
 * @brief   WIFI Source File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-05
 */


#include "wifi.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"


static volatile uint8_t wifi_reconnect_counter;
static EventGroupHandle_t wifi_event_group;


/**
 * @brief             The WIFI event handler.
 *
 * @param arg         Arguments sent by event registrations.
 * @param event_base  The base code of the event.
 * @param event_id    The id of the event.
 * @param event_data  Data sent from the event.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  esp_err_t esp_err = ESP_OK;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_err = esp_wifi_connect();
    if (esp_err != ESP_OK) {
      ESP_LOGE(WIFI_TAG, "Connect failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (wifi_reconnect_counter < WIFI_MAX_RECONNECTIONS) {
      wifi_reconnect_counter++;

      ESP_LOGI(WIFI_TAG, "Reconnect attempt %d", wifi_reconnect_counter);
      esp_err = esp_wifi_connect();
      if (esp_err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Connect failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
      }
    } else {
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(WIFI_TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    wifi_reconnect_counter = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}


/**
 * @brief             Waits until the wifi_event_handler sets the WIFI_CONNECTED_BIT or the WIFI_FAIL_BIT and logs the info.
 */
static void wifi_check_connection()
{
  // Waits until either the WIFI_CONNECTED_BIT or the WIFI_FAIL_BIT is set. The bits are set by the wifi_event_handler.
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(WIFI_TAG, "Connected to AP with SSID: %s", WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(WIFI_TAG, "Failed to connect to AP with SSID: %s", WIFI_SSID);
  } else {
    ESP_LOGE(WIFI_TAG, "Unexpected event");
  }

  // Clears the bits and prepares for the next iteration.
  xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
}


/**
 * @brief             The WIFI task function. Makes a connection to an AP and preserves it.
 */
void wifi_task()
{
  esp_err_t esp_err = ESP_OK;

  wifi_event_group = xEventGroupCreate();

  esp_err = esp_netif_init();
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "TCP/IP stack initialization failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_err = esp_event_loop_create_default();
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Event loop creation failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err = esp_wifi_init(&wifi_init_config);
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Driver initialization failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_event_handler_instance_t instance_any_id;
  esp_err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "WIFI event instance register failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_event_handler_instance_t instance_got_ip;
  esp_err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "IP event instance register failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASS,
      .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      .pmf_cfg = {
        .capable = true,
        .required = false
      }
    }
  };
  esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Operation mode setup failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Configuration failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_err = esp_wifi_start();
  if (esp_err != ESP_OK) {
    ESP_LOGE(WIFI_TAG, "Start failed with error 0x%x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  wifi_check_connection();

  while(1) {
    if (wifi_reconnect_counter == WIFI_MAX_RECONNECTIONS) {
      wifi_reconnect_counter = 0;
      esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL, 0, 100 / portTICK_PERIOD_MS);
      wifi_check_connection();
    }

    vTaskDelay(WIFI_CHECK_CONNECTION_PERIOD_MS / portTICK_PERIOD_MS);
  }
}
