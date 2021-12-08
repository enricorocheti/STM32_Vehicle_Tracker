/*
 * File name: 			mpu6050.h
 * File description: 	Functions prototypes to handle the MPU-6050 IMU
 * Author name: 		Enrico Oliveira Rocheti
 * Revision date: 		07/12/2021
*/

#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "global.h"

/* FIR filter defines */
#define ARM_MATH_CM4
#include <arm_math.h>

#define SAMPLE_SIZE		100
#define COEFF_SIZE		8
#define GRAVITY_ACC		9.81

/* I2C defines */
#define MPU6050_ADDR 		0xD0
#define REG_WHO_AM_I		0x75
#define REG_PWR_MGMT_1 		0x6B
#define REG_SMPLRT_DIV 		0x19
#define REG_ACCEL_CONFIG 	0x1C
#define REG_GYRO_CONFIG 	0x1B
#define REG_ACCEL_XOUT_H 	0x3B
#define REG_GYRO_XOUT_H 	0x43


/*
 * Method name: 		InitImu
 * Method description: 	Config IMU registers using I2C
 * Input params: 		n/a
 * Output params: 		return 0:		OK
 * 						return -1: 		ERROR
*/
int InitImu( void );


/*
 * Method name: 		GetImuData
 * Method description: 	Get IMU data, passing it through a FIR filter
 * Input params: 		data: struct to receive the IMU's data
 * Output params: 		n/a
*/
void GetImuData( struct sensorData * data );


/*
 * Method name: 		LowPowerModeImu
 * Method description: 	Set IMU to low-power mode
 * Input params: 		n/a
 * Output params: 		n/a
*/
void LowPowerModeImu( void );


/*
 * Method name: 		FIRFilterData
 * Method description: 	Apply FIR filter to data
 * Input params: 		filter: filter pre configured struct
 * 						data:	data to be filterd
 * Output params: 		n/a
*/
void FIRFilterData( arm_fir_instance_f32 * filter, float * data );


/*
 * Method name: 		CalculateAverage
 * Method description: 	Calculate the average of a float vector
 * Input params: 		data: float data
 * Output params: 		single avarage data
*/
float CalculateAverage( float * data );


#endif
