/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x30A0A7FB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB8-BOOT0     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB8-BOOT0     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
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

	// Read 6 BYTES of data starting from ACCEL_XOUT_H register
	HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_ACCEL_XOUT_H, 1, accelData, 6, 500 );

	// Read 6 BYTES of data starting from GYRO_XOUT_H register
	HAL_I2C_Mem_Read( &hi2c1, MPU6050_ADDR, REG_GYRO_XOUT_H, 1, gyroData, 6, 500 );

	/*** convert the RAW values into acceleration in 'g'
	     we have to divide according to the Full scale value set in FS_SEL
	     I have configured FS_SEL = 0. So I am dividing by 16384.0
	     for more details check ACCEL_CONFIG Register              ****/

	rawAccelX = (int16_t) ( accelData[0] << 8 | accelData[1] );
	rawAccelY = (int16_t) ( accelData[2] << 8 | accelData[3] );
	rawAccelZ = (int16_t) ( accelData[4] << 8 | accelData[5] );
	data->accelX = rawAccelX / 16384.0;
	data->accelY = rawAccelY / 16384.0;
	data->accelZ = rawAccelZ / 16384.0;

	rawGyroX = (int16_t) ( gyroData[0] << 8 | gyroData[1] );
	rawGyroY = (int16_t) ( gyroData[2] << 8 | gyroData[3] );
	rawGyroZ = (int16_t) ( gyroData[4] << 8 | gyroData[5] );
	data->gyroX = rawGyroX / 16384.0;
	data->gyroY = rawGyroY / 16384.0;
	data->gyroZ = rawGyroZ / 16384.0;
}

void LowPowerModeImu( void )
{
	uint8_t buffer = 0b01000000; // bit 6 set 1 = low power sleep mode
	HAL_I2C_Mem_Write( &hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, 1, &buffer, 1, 500 );
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
