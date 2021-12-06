#include "mpu6050.h"


int InitImu( void )
{
	uint8_t buffer = 0x00;
	int ret = 0;

	/* Check device's fixed register, 0x68 will be returned if everything goes well */
	HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_WHO_AM_I, 1, &buffer, 1, 500 );

	if ( buffer == 0x68 )
	{
		// Wake the device up
		buffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &buffer, 1, 500 );

		// Set gyroscope sample rate to 1KHz, same as the accelerometer
		buffer = 0x07;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_SMPLRT_DIV, 1, &buffer, 1, 500 );

		// Set accelerometer range to ± 2g
		buffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_ACCEL_CONFIG, 1, &buffer, 1, 500 );

		// Set gyroscope range to ± 250 °/s
		buffer = 0x00;
		HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_GYRO_CONFIG, 1, &buffer, 1, 500 );

		ret = 1;
	}
	else
	{
		ret = -1;
	}

	return ret;
}


void GetImuData( struct sensorData * data )
{
	uint8_t accelData[6];
	uint8_t gyroData[6];
	int16_t rawAccelX = 0, rawAccelY = 0, rawAccelZ = 0;
	int16_t rawGyroX = 0, rawGyroY = 0, rawGyroZ = 0;
	float accelX[SAMPLE_SIZE], accelY[SAMPLE_SIZE], accelZ[SAMPLE_SIZE];
	float gyroX[SAMPLE_SIZE], gyroY[SAMPLE_SIZE], gyroZ[SAMPLE_SIZE];

	/*** convert the RAW values into acceleration in 'g'
	     we have to divide according to the Full scale value set in FS_SEL
	     I have configured FS_SEL = 0. So I am dividing by 16384.0
	     for more details check ACCEL_CONFIG Register              ****/

	for ( uint8_t i = 0; i < SAMPLE_SIZE; i++ )
	{
		/* Read 6 BYTES of data starting from ACCEL_XOUT_H register */
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
		/* Read 6 BYTES of data starting from GYRO_XOUT_H register */
		HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_GYRO_XOUT_H, 1, gyroData, 6, 500 );
		rawGyroX = (int16_t) ( gyroData[0] << 8 | gyroData[1] );
		rawGyroY = (int16_t) ( gyroData[2] << 8 | gyroData[3] );
		rawGyroZ = (int16_t) ( gyroData[4] << 8 | gyroData[5] );

		gyroX[i] = rawGyroX / 16384.0;
		gyroY[i] = rawGyroY / 16384.0;
		gyroZ[i] = rawGyroZ / 16384.0;
	}

	data->accelX = CalculateAverage( accelX );
	data->accelY = CalculateAverage( accelY );
	data->accelZ = CalculateAverage( accelZ );
	data->gyroX = CalculateAverage( gyroX );
	data->gyroY = CalculateAverage( gyroY );
	data->gyroZ = CalculateAverage( gyroZ );

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
	float filteredSample[SAMPLE_SIZE];
	arm_fir_f32( filter, data, &filteredSample[0], SAMPLE_SIZE );
	memcpy( data, filteredSample, SAMPLE_SIZE );
}

float CalculateAverage( float * data )
{
	float average = 0.0;

	// Discard first 10 data from filter oscilation
	for ( uint8_t i = 10; i < SAMPLE_SIZE; i++ )
	{
		average += data[i];
	}
	average /= SAMPLE_SIZE - 10;

	return average;
}


void LowPowerModeImu( void )
{
	uint8_t buffer = 0b01000000; // bit 6 set 1 = low power sleep mode
	HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &buffer, 1, 500 );
}
