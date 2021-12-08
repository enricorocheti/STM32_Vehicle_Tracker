/*
 * File name: 			mpu6050.c
 * File description: 	Functions to handle the MPU-6050 IMU
 * Author name: 		Enrico Oliveira Rocheti
 * Revision date: 		07/12/2021
*/


#include "mpu6050.h"


int InitImu( void )
{
	/* Function based on the following references:
	 *
	 * https://controllerstech.com/how-to-interface-mpu6050-gy-521-with-stm32/
	 *
	 */

	uint8_t uiBuffer = 0x00;
	int iRet;

	/* Check device's fixed register, 0x68 will be returned if everything goes well */
	HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_WHO_AM_I, 1, &uiBuffer, 1, 500 );

	if ( uiBuffer == 0x68 )
	{
		/* Wake the device up */
		uiBuffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &uiBuffer, 1, 500 );

		/* Set gyroscope sample rate to 1KHz, same as the accelerometer */
		uiBuffer = 0x07;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_SMPLRT_DIV, 1, &uiBuffer, 1, 500 );

		/* Set accelerometer range to ± 2g */
		uiBuffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_ACCEL_CONFIG, 1, &uiBuffer, 1, 500 );

		/* Set gyroscope range to ± 250 °/s */
		uiBuffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_GYRO_CONFIG, 1, &uiBuffer, 1, 500 );

		iRet = 0;
	}
	else
	{
		iRet = 1;
	}

	return iRet;
}


void GetImuData( struct sensorData * data )
{
	/* Function based on the following references:
	 *
	 * https://controllerstech.com/how-to-interface-mpu6050-gy-521-with-stm32/
	 * https://notblackmagic.com/bitsnpieces/digital-filters/
	 *
	 */

	uint8_t accelData[6];
	uint8_t gyroData[6];
	int16_t rawAccelX = 0, rawAccelY = 0, rawAccelZ = 0;
	int16_t rawGyroX = 0, rawGyroY = 0, rawGyroZ = 0;
	float accelX[SAMPLE_SIZE], accelY[SAMPLE_SIZE], accelZ[SAMPLE_SIZE];
	float gyroX[SAMPLE_SIZE], gyroY[SAMPLE_SIZE], gyroZ[SAMPLE_SIZE];

	for ( uint8_t i = 0; i < SAMPLE_SIZE; i++ )
	{
		/* Read 6 bytes of data starting from ACCEL_XOUT_H register */
		HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_ACCEL_XOUT_H, 1, accelData, 6, 500 );
		rawAccelX = (int16_t) ( accelData[0] << 8 | accelData[1] );
		rawAccelY = (int16_t) ( accelData[2] << 8 | accelData[3] );
		rawAccelZ = (int16_t) ( accelData[4] << 8 | accelData[5] );

		accelX[i] = rawAccelX * GRAVITY_ACC / 16384.0;
		accelY[i] = rawAccelY * GRAVITY_ACC / 16384.0;
		accelZ[i] = rawAccelZ * GRAVITY_ACC / 16384.0;
	}

	for ( uint8_t i = 0; i < SAMPLE_SIZE; i++ )
	{
		/* Read 6 bytes of data starting from GYRO_XOUT_H register */
		HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_GYRO_XOUT_H, 1, gyroData, 6, 500 );
		rawGyroX = (int16_t) ( gyroData[0] << 8 | gyroData[1] );
		rawGyroY = (int16_t) ( gyroData[2] << 8 | gyroData[3] );
		rawGyroZ = (int16_t) ( gyroData[4] << 8 | gyroData[5] );

		gyroX[i] = rawGyroX / 131.0;
		gyroY[i] = rawGyroY / 131.0;
		gyroZ[i] = rawGyroZ / 131.0;
	}

	/* Float FIR Filter, low-pass 10Hz cutoff */
	float firStateF32[COEFF_SIZE + SAMPLE_SIZE - 1];
	float firCoeffF32[COEFF_SIZE] = { 	0.020623, 0.065507, 0.166590, 0.247822,
										0.247822, 0.166590, 0.065507, 0.020623 };
	arm_fir_instance_f32 armFIRInstanceF32;
	arm_fir_init_f32( &armFIRInstanceF32, COEFF_SIZE, firCoeffF32, firStateF32, SAMPLE_SIZE );

	FIRFilterData( &armFIRInstanceF32, &accelX[0] );
	FIRFilterData( &armFIRInstanceF32, &accelY[0] );
	FIRFilterData( &armFIRInstanceF32, &accelZ[0] );
	data->accelX = CalculateAverage( accelX );
	data->accelY = CalculateAverage( accelY );
	data->accelZ = CalculateAverage( accelZ );

	FIRFilterData( &armFIRInstanceF32, &gyroX[0] );
	FIRFilterData( &armFIRInstanceF32, &gyroY[0] );
	FIRFilterData( &armFIRInstanceF32, &gyroZ[0] );
	data->gyroX = CalculateAverage( gyroX );
	data->gyroY = CalculateAverage( gyroY );
	data->gyroZ = CalculateAverage( gyroZ );
}


void FIRFilterData( arm_fir_instance_f32 * filter, float * data )
{
	float fFilteredSample[SAMPLE_SIZE];
	arm_fir_f32( filter, data, &fFilteredSample[0], SAMPLE_SIZE );
	memcpy( data, fFilteredSample, SAMPLE_SIZE );
}

float CalculateAverage( float * data )
{
	float fAverage = 0.0;

	/* Discard first 10 data points because of filter oscilation on it's first samples */
	for ( uint8_t i = 10; i < SAMPLE_SIZE; i++ )
	{
		fAverage += data[i];
	}
	fAverage /= (SAMPLE_SIZE - 10);

	return fAverage;
}


void LowPowerModeImu( void )
{
	/* Bit 6 set to 1 -> low power sleep mode */
	uint8_t uiBuffer = 0b01000000;
	HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &uiBuffer, 1, 500 );
}
