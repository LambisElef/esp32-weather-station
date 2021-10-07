/**
 * @file    bme.h
 *
 * @brief   BME Header File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-03
 */


#ifndef _BME_H_
#define _BME_H_


#include "bme280.h"

#include <stdint.h>


#define BME_TAG                       " BME"

#define BME280_FAIL                   (-7)
#define BME280_FLOAT_ENABLE

#define BME_SAMPLING_PERIOD_MS        (10000)
#define BME_HTTP_SEND_RETRIES         (5)
#define BME_HTTP_SEND_RETRY_WAIT_MS   (100)

#define BME_TASK_NAME                "bme"
#define BME_TASK_PRIORITY            (tskIDLE_PRIORITY + 1)
#define BME_TASK_STACK_SIZE          (2048)


void bme_delay(uint32_t period, void *intf_ptr);


int8_t bme_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);


int8_t bme_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);


void bme_task();


#endif /* _BME_H_ */
