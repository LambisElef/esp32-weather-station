/**
 * @file    wifi.h
 *
 * @brief   WIFI Header File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-05
 */


#ifndef _WIFI_H_
#define _WIFI_H_


#define WIFI_TAG                        "WIFI"

#define WIFI_SSID                       "Your Wifi SSID"
#define WIFI_PASS                       "Your WiFi Password"

#define WIFI_CHECK_CONNECTION_PERIOD_MS (10000)
#define WIFI_CONNECTED_BIT              (BIT0)
#define WIFI_FAIL_BIT                   (BIT1)
#define WIFI_MAX_RECONNECTIONS          (10)

#define WIFI_TASK_NAME                  "wifi"
#define WIFI_TASK_PRIORITY              (tskIDLE_PRIORITY + 1)
#define WIFI_TASK_STACK_SIZE            (8192)


void wifi_task();


#endif /* _WIFI_H_ */
