/**
 * @file    http.h
 *
 * @brief   HTTP Header File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-05
 */


#ifndef _HTTP_H_
#define _HTTP_H_


#include <stdint.h>


#define HTTP_TAG                      "HTTP"

#define HTTP_TASK_NAME                "http"
#define HTTP_TASK_PRIORITY            (tskIDLE_PRIORITY + 1)
#define HTTP_TASK_STACK_SIZE          (8192)

#define HTTP_FIELD_SIZE               (256)
#define HTTP_POLL_PERIOD_MS           (5000)

#define HTTP_POST_URL                 "https://<Your InfluxDB Address:Port>/write?db=<Your InfluxDB DB Name>&u=<Your InfluxDB Username>&p=<Your InfluxDB Password>"
#define HTTP_TIMEOUT_MS               (10000)

typedef enum {
  HTTP_DATA_OK,
  HTTP_DATA_PENDING
} http_data_en;


http_data_en http_send(char *data, uint32_t data_len);


void http_task();

#endif /* _HTTP_H_ */
