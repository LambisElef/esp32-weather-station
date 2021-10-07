/**
 * @file    i2c.h
 *
 * @brief   I2C Header File
 *
 * @author  Charalampos Eleftheriadis
 * @version 0.1
 * @date    2021-08-03
 */


#ifndef _I2C_H_
#define _I2C_H_


#include "driver/i2c.h"


#define I2C_TAG                       (" I2C")

#define I2C_PORT                      (I2C_NUM_0)
#define I2C_SCL                       (GPIO_NUM_19)
#define I2C_SDA                       (GPIO_NUM_23)
#define I2C_SPEED                     (1e6)
#define I2C_WAIT_MS                   (10)


#endif /* _I2C_H_ */
