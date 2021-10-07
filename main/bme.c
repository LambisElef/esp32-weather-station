/**
 * @file    bme.c
 *
 * @brief   BME Source File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-03
 */


#include "bme.h"

#include "esp_log.h"

#include "i2c.h"
#include "http.h"

#include <string.h>


/**
 * @brief           Delays the system.
 *
 * @param period    The microseconds to delay the system for.
 * @param intf_ptr  The sensor address.
 */
void bme_delay(uint32_t period, void *intf_ptr)
{
  vTaskDelay(period / 1000 / portTICK_PERIOD_MS);
}


/**
 * @brief           Reads from the sensor via I2C.
 *
 * @param reg_addr  The register address.
 * @param reg_data  The data read from the sensor.
 * @param len       The number of bytes to read.
 * @param intf_ptr  The sensor address.
 *
 * @return        - BME280_OK
 *                - BME280_FAIL
 */
int8_t bme_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  esp_err_t esp_err = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  esp_err = i2c_master_start(cmd);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_write_byte(cmd, (*(uint8_t *)intf_ptr << 1) | I2C_MASTER_WRITE, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_write_byte(cmd, reg_addr, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

//  esp_err = i2c_master_stop(cmd);
//  if (esp_err != ESP_OK) {
//    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
//    return BME280_E_COMM_FAIL;
//  }

  esp_err = i2c_master_start(cmd);
  if (esp_err != ESP_OK) {
   ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
   return BME280_FAIL;
  }

  esp_err = i2c_master_write_byte(cmd, (*(uint8_t *)intf_ptr << 1) | I2C_MASTER_READ, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  if (len > 1) {
    esp_err = i2c_master_read(cmd, reg_data, len-1, I2C_MASTER_ACK);
    if (esp_err != ESP_OK) {
      ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
      return BME280_FAIL;
    }
  }

  esp_err = i2c_master_read_byte(cmd, reg_data+len-1, I2C_MASTER_NACK);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_stop(cmd);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_cmd_begin(I2C_PORT, cmd, I2C_WAIT_MS / portTICK_PERIOD_MS);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Read failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  i2c_cmd_link_delete(cmd);

  return BME280_OK;
}


/**
 * @brief           Writes to the sensor via I2C.
 *
 * @param reg_addr  The register address.
 * @param reg_data  The data to be written.
 * @param len       The number of bytes to write.
 * @param intf_ptr  The sensor address.
 *
 * @return        - BME280_OK
 *                - BME280_FAIL
 */
int8_t bme_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  esp_err_t esp_err = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  esp_err = i2c_master_start(cmd);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_write_byte(cmd, (*(uint8_t *)intf_ptr << 1) | I2C_MASTER_WRITE, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_write_byte(cmd, reg_addr, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_write(cmd, reg_data, len, 1);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_stop(cmd);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  esp_err = i2c_master_cmd_begin(I2C_PORT, cmd, I2C_WAIT_MS / portTICK_PERIOD_MS);
  if (esp_err != ESP_OK) {
    ESP_LOGE(BME_TAG, "Write failed with error 0x%x", esp_err);
    return BME280_FAIL;
  }

  i2c_cmd_link_delete(cmd);

  return BME280_OK;
}


/**
 * @brief           The BME sensor task function. Initializes the sensor and then polls it for data and sends it via HTTP.
 */
void bme_task()
{
  int8_t bme_err = BME280_OK;
  struct bme280_data bme_data;

  uint8_t bme_addr = BME280_I2C_ADDR_PRIM;
  struct bme280_dev bme = {
    .intf = BME280_I2C_INTF,
    .intf_ptr = &bme_addr,
    .read = bme_read,
    .write = bme_write,
    .delay_us = bme_delay
  };

  bme_err = bme280_init(&bme);
  if (bme_err != BME280_OK) {
    ESP_LOGE(BME_TAG, "Initialization failed with code %d", bme_err);
    return;
  }

  bme.settings.osr_h = BME280_OVERSAMPLING_1X;
  bme.settings.osr_p = BME280_OVERSAMPLING_16X;
  bme.settings.osr_t = BME280_OVERSAMPLING_2X;
  bme.settings.filter = BME280_FILTER_COEFF_16;

  uint8_t settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

  bme_err = bme280_set_sensor_settings(settings_sel, &bme);
  if (bme_err != BME280_OK) {
    ESP_LOGE(BME_TAG, "Configuration failed with code %d", bme_err);
    return;
  }

  // Discards the first measurement.
  bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
  bme.delay_us(40000, bme.intf_ptr);
  bme280_get_sensor_data(BME280_ALL, &bme_data, &bme);
  bme.delay_us(BME_SAMPLING_PERIOD_MS * 1000, bme.intf_ptr);

  while (1) {
    bme_err = bme280_set_sensor_mode(BME280_FORCED_MODE, &bme);
    if (bme_err != BME280_OK) {
      ESP_LOGE(BME_TAG, "Mode setup failed with code %d", bme_err);
      break;
    }

    // Waits for measurement to complete and gets data.
    bme.delay_us(40000, bme.intf_ptr);
    bme_err = bme280_get_sensor_data(BME280_ALL, &bme_data, &bme);
    if (bme_err != BME280_OK) {
      ESP_LOGE(BME_TAG, "Data acquisition failed");
      break;
    }

    char data[HTTP_FIELD_SIZE];
    sprintf(data, "sensor,location=home temperature=%0.2lf,pressure=%0.2lf,humidity=%0.2lf", bme_data.temperature,  0.01 * bme_data.pressure, bme_data.humidity);
    int i = 0;
    for (i=0; i< BME_HTTP_SEND_RETRIES; i++) {
      http_data_en err = http_send(data, strlen(data));
      if (err == HTTP_DATA_OK) {
        break;
      } else {
        bme.delay_us(BME_HTTP_SEND_RETRY_WAIT_MS * 1000, bme.intf_ptr);
      }
    }

    printf("%0.2lf deg C, %0.2lf hPa, %0.2lf%%\n", bme_data.temperature,  0.01 * bme_data.pressure, bme_data.humidity);

    bme.delay_us((BME_SAMPLING_PERIOD_MS - i*BME_HTTP_SEND_RETRY_WAIT_MS) * 1000, bme.intf_ptr);
  }
}
