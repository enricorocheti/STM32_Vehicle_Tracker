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
uint8_t GVLowPowerFlag = 0;
uint8_t GVNetworkStatus = 0;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for TLowPowerMode */
osThreadId_t TLowPowerModeHandle;
const osThreadAttr_t TLowPowerMode_attributes = {
  .name = "TLowPowerMode",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 1024 * 4
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
void FuncLowPowerMode(void *argument);
void FuncNwConnect(void *argument);
void FuncSensorData(void *argument);
void FuncNwSendData(void *argument);

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

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of TLowPowerMode */
  TLowPowerModeHandle = osThreadNew(FuncLowPowerMode, NULL, &TLowPowerMode_attributes);

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
	  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	  HAL_GPIO_TogglePin(LD_GPS_GPIO_Port, LD_GPS_Pin);
	  HAL_GPIO_TogglePin(LD_SIM_GPIO_Port, LD_SIM_Pin);
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_FuncLowPowerMode */
/**
* @brief Function implementing the TLowPowerMode thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FuncLowPowerMode */
void FuncLowPowerMode(void *argument)
{
  /* USER CODE BEGIN FuncLowPowerMode */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END FuncLowPowerMode */
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
    osDelay(1);
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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
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
    osDelay(1);
  }
  /* USER CODE END FuncNwSendData */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
