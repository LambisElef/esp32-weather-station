/**
 * @file    main.c
 *
 * @brief   The main file of the project.
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-03
 */


#include "esp_log.h"
#include "nvs_flash.h"

#include "bme.h"
#include "http.h"
#include "i2c.h"
#include "wifi.h"


#define MAIN_TAG                      "MAIN"


static TaskHandle_t bme_task_handle = NULL;
static TaskHandle_t http_task_handle = NULL;
static TaskHandle_t wifi_task_handle = NULL;


void app_main(void)
{
  esp_err_t esp_err = ESP_OK;

  // Initializes the NVS. Required by the WIFI driver.
  esp_err = nvs_flash_init();
  if (esp_err != ESP_OK) {
    ESP_LOGE(MAIN_TAG, "NVS initial failed with code %x [%s]", esp_err, esp_err_to_name(esp_err));
    return;
  }

  // Creates the WIFI task.
  xTaskCreate(wifi_task, WIFI_TASK_NAME, WIFI_TASK_STACK_SIZE, NULL, WIFI_TASK_PRIORITY, &wifi_task_handle);


  i2c_config_t i2c_config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_SDA,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_SCL,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_SPEED
  };

  esp_err = i2c_param_config(I2C_PORT, &i2c_config);
  if (esp_err != ESP_OK) {
    ESP_LOGE(I2C_TAG, "Configuration failed with code %x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  esp_err = i2c_driver_install(I2C_PORT, i2c_config.mode, 0, 0, 0);
  if (esp_err != ESP_OK) {
    ESP_LOGE(I2C_TAG, "Driver initialization failed with code %x [%s]", esp_err, esp_err_to_name(esp_err));
  }

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // Creates the BME sensor task.
  xTaskCreate(bme_task, BME_TASK_NAME, BME_TASK_STACK_SIZE, NULL, BME_TASK_PRIORITY, &bme_task_handle);

  // Creates the HTTP task.
  xTaskCreate(http_task, HTTP_TASK_NAME, HTTP_TASK_STACK_SIZE, NULL, HTTP_TASK_PRIORITY, &http_task_handle);

  while (1) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

