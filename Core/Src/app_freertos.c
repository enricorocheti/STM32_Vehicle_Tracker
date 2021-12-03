/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

#include "usart.h"
#include "i2c.h"

#include "global.h"
#include "sim808.h"
#include "mpu6050.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// 1 = MODO LP OFF
// 2 = MODO LP ON
uint8_t uiGVLowPowerFlag = 0x02;
uint8_t uiGVNetworkStatus = 0;
struct sensorData xGVDataPacket;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for TaskLowPwrMode */
osThreadId_t TaskLowPwrModeHandle;
const osThreadAttr_t TaskLowPwrMode_attributes = {
  .name = "TaskLowPwrMode",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 12000 * 4
};
/* Definitions for TaskNwConnect */
osThreadId_t TaskNwConnectHandle;
const osThreadAttr_t TaskNwConnect_attributes = {
  .name = "TaskNwConnect",
  .priority = (osPriority_t) osPriorityNormal2,
  .stack_size = 1024 * 4
};
/* Definitions for TaskSensorData */
osThreadId_t TaskSensorDataHandle;
const osThreadAttr_t TaskSensorData_attributes = {
  .name = "TaskSensorData",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for TaskNwSendData */
osThreadId_t TaskNwSendDataHandle;
const osThreadAttr_t TaskNwSendData_attributes = {
  .name = "TaskNwSendData",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for SensorDataTimer */
osTimerId_t SensorDataTimerHandle;
const osTimerAttr_t SensorDataTimer_attributes = {
  .name = "SensorDataTimer"
};
/* Definitions for MutexSerialComSIM808 */
osMutexId_t MutexSerialComSIM808Handle;
const osMutexAttr_t MutexSerialComSIM808_attributes = {
  .name = "MutexSerialComSIM808"
};
/* Definitions for MutexDataPacket */
osMutexId_t MutexDataPacketHandle;
const osMutexAttr_t MutexDataPacket_attributes = {
  .name = "MutexDataPacket"
};
/* Definitions for MutexNetworkStatus */
osMutexId_t MutexNetworkStatusHandle;
const osMutexAttr_t MutexNetworkStatus_attributes = {
  .name = "MutexNetworkStatus"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void FuncLowPwrMode(void *argument);
void FuncNwConnect(void *argument);
void FuncSensorData(void *argument);
void FuncNwSendData(void *argument);
void CallbackTimer(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of MutexSerialComSIM808 */
  MutexSerialComSIM808Handle = osMutexNew(&MutexSerialComSIM808_attributes);

  /* creation of MutexDataPacket */
  MutexDataPacketHandle = osMutexNew(&MutexDataPacket_attributes);

  /* creation of MutexNetworkStatus */
  MutexNetworkStatusHandle = osMutexNew(&MutexNetworkStatus_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of SensorDataTimer */
  SensorDataTimerHandle = osTimerNew(CallbackTimer, osTimerPeriodic, NULL, &SensorDataTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of TaskLowPwrMode */
  TaskLowPwrModeHandle = osThreadNew(FuncLowPwrMode, NULL, &TaskLowPwrMode_attributes);

  /* creation of TaskNwConnect */
  TaskNwConnectHandle = osThreadNew(FuncNwConnect, NULL, &TaskNwConnect_attributes);

  /* creation of TaskSensorData */
  TaskSensorDataHandle = osThreadNew(FuncSensorData, NULL, &TaskSensorData_attributes);

  /* creation of TaskNwSendData */
  TaskNwSendDataHandle = osThreadNew(FuncNwSendData, NULL, &TaskNwSendData_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
	  osDelay(10000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_FuncLowPwrMode */
/**
* @brief Function implementing the TaskLowPwrMode thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FuncLowPwrMode */
void FuncLowPwrMode(void *argument)
{
  /* USER CODE BEGIN FuncLowPwrMode */
	PowerOnSimRadio();
  /* Infinite loop */
  for(;;)
  {
	  /* Blink LEDs to show that the system is on low-power mode */
	  if ( uiGVLowPowerFlag == 0x02 )
	  {
		  HAL_GPIO_TogglePin(LD_GPS_GPIO_Port, LD_GPS_Pin);
		  HAL_GPIO_TogglePin(LD_SIM_GPIO_Port, LD_SIM_Pin);
	  }

	  // Wait forever until either thread flag 0 or 1 is set.
	  osThreadFlagsWait( 0x00000003, osFlagsWaitAny | osFlagsNoClear, 500 );
	  uint32_t flags = osThreadFlagsGet();
	  if ( flags == 0x00000001 )
	  {
		  /* Turn off low-power mode */
		  osMutexAcquire( MutexSerialComSIM808Handle, osWaitForever );
		  InitSimRadio();
		  osMutexRelease( MutexSerialComSIM808Handle );
		  InitImu();

		  uiGVLowPowerFlag = 0x01;

		  /* Turn off system LEDs */
		  HAL_GPIO_WritePin(LD_SIM_GPIO_Port, LD_SIM_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(LD_GPS_GPIO_Port, LD_GPS_Pin, GPIO_PIN_RESET);
	  }
	  else if ( flags == 0x00000002 )
	  {
		  /* Turn on low-power mode */
		  osMutexAcquire( MutexSerialComSIM808Handle, osWaitForever );
		  LowPowerModeSimRadio();
		  osMutexRelease( MutexSerialComSIM808Handle );
		  LowPowerModeImu();

		  uiGVLowPowerFlag = 0x02;
	  }
	  flags = osThreadFlagsClear( flags );

	  osDelay(10);
  }
  /* USER CODE END FuncLowPwrMode */
}

/* USER CODE BEGIN Header_FuncNwConnect */
/**
* @brief Function implementing the TaskNwConnect thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FuncNwConnect */
void FuncNwConnect(void *argument)
{
  /* USER CODE BEGIN FuncNwConnect */
  /* Infinite loop */
  for(;;)
  {
	  if ( uiGVLowPowerFlag == 0x01 )
	  {
		  if ( uiGVNetworkStatus == 0 )
		  {
			  int ret = 1;//JoinSimRadioNetwork();
			  if ( ret == 1 )
			  {
				  /* Successfully joined network */
				  uiGVNetworkStatus = 1;
				  HAL_GPIO_WritePin( LD_SIM_GPIO_Port, LD_SIM_Pin, GPIO_PIN_SET );
			  }
			  else
			  {
				  /* Couldn't join network */
				  HAL_GPIO_WritePin( LD_SIM_GPIO_Port, LD_SIM_Pin, GPIO_PIN_RESET );
			  }
		  }
		  osDelay(10000);
	  }
	  osDelay(10);
  }
  /* USER CODE END FuncNwConnect */
}

/* USER CODE BEGIN Header_FuncSensorData */
/**
* @brief Function implementing the TaskSensorData thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FuncSensorData */
void FuncSensorData(void *argument)
{
  /* USER CODE BEGIN FuncSensorData */
	osTimerStart( SensorDataTimerHandle, 10000U );
  /* Infinite loop */
  for(;;)
  {
	  if ( uiGVLowPowerFlag == 0x01 )
	  {
		  /* 10 seconds timer will set this flag */
		  osThreadFlagsWait( 0x00000001, osFlagsWaitAny, osWaitForever );

		  /* Changing xGVDataPacket variable */
		  osMutexAcquire( MutexDataPacketHandle, osWaitForever );

		  memset( &xGVDataPacket, 0, sizeof(xGVDataPacket) );

		  osMutexAcquire( MutexSerialComSIM808Handle, osWaitForever );
		  GetGpsData( &xGVDataPacket );
		  osMutexRelease( MutexSerialComSIM808Handle );

		  if ( xGVDataPacket.gpsStatus == 'A' )
		  {
			  /* GPS data is not valid */
			  HAL_GPIO_WritePin(LD_GPS_GPIO_Port, LD_GPS_Pin, GPIO_PIN_RESET);
		  }
		  else
		  {
			  /* GPS data is valid */
			  HAL_GPIO_WritePin(LD_GPS_GPIO_Port, LD_GPS_Pin, GPIO_PIN_SET);
		  }

		  GetImuData( &xGVDataPacket );

		  /* Done with the xGVDataPacket variable */
		  osMutexRelease( MutexDataPacketHandle );

		  /* Tells TaskNwSendData that there is new data to be sent */
		  osThreadFlagsSet( TaskNwSendDataHandle, 0x00000001 );
	  }
	  osDelay(10);
  }
  /* USER CODE END FuncSensorData */
}

/* USER CODE BEGIN Header_FuncNwSendData */
/**
* @brief Function implementing the TaskNwSendData thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FuncNwSendData */
void FuncNwSendData(void *argument)
{
  /* USER CODE BEGIN FuncNwSendData */
  /* Infinite loop */
  for(;;)
  {
	  if ( uiGVLowPowerFlag == 0x01 )
	  {
		  osThreadFlagsWait( 0x00000001, osFlagsWaitAny, osWaitForever );
		  //osMutexAcquire( MutexDataPacketHandle, osWaitForever );
		  //osMutexRelease( MutexDataPacketHandle );
	  }
	  osDelay(10);
  }
  /* USER CODE END FuncNwSendData */
}

/* CallbackTimer function */
void CallbackTimer(void *argument)
{
  /* USER CODE BEGIN CallbackTimer */

	osThreadFlagsSet( TaskSensorDataHandle, 0x00000001 );
	//if ( (ret >> 7) & 1 )
	//{
		/* If bit 8 is set, there's an error on osThreadFlagsSet execution */
	//}

  /* USER CODE END CallbackTimer */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if ( GPIO_Pin == B1_Pin )
	{
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		if ( uiGVLowPowerFlag == 0x01 )
		{
			/* Ativa modo low-power */
			osThreadFlagsSet( TaskLowPwrModeHandle, 0x00000002 );
		}
		else if ( uiGVLowPowerFlag == 0x02 )
		{
			/* Desativa modo low-power */
			osThreadFlagsSet( TaskLowPwrModeHandle, 0x00000001 );
		}
	}
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
