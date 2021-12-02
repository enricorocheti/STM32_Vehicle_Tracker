#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif


//#include <stdio.h>
//#include <string.h>
#include "i2c.h"
#include "global.h"
//#include "usart.h"


#define MPU6050_ADDR 		0xD0

#define REG_WHO_AM_I		0x75
#define REG_PWR_MGMT_1 		0x6B
#define REG_SMPLRT_DIV 		0x19
#define REG_ACCEL_CONFIG 	0x1C
#define REG_GYRO_CONFIG 	0x1B
#define REG_ACCEL_XOUT_H 	0x3B
#define REG_GYRO_XOUT_H 	0x43


int InitImu( void );

void GetImuData( struct sensorData * data );

void LowPowerModeImu( void );

void FilterImuData( void );

#endif
