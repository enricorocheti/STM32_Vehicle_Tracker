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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// I2C regs


// General commands
#define AT_SIM_SET_FULLFUNC		"AT+CFUN=1\r\n"		// Set Phone Full functionality
#define AT_SIM_GET_CPIN			"AT+CPIN?\r\n"		// Check if SIM card is inserted
#define AT_SIM_SET_ECHOOFF		"ATE0\r\n"			// Disable commands echo

// GPS commands
#define AT_GPS_SET_PWRON		"AT+CGPSPWR=1\r\n"	// Power On GPS
#define AT_GPS_SET_PWROFF		"AT+CGPSPWR=0\r\n"	// Power Off GPS
#define AT_GPS_SET_HOTMODE		"AT+CGPSRST=1\r\n"	// Set GPS to hot start
#define AT_GPS_GET_DATA			"AT+CGPSINF=32\r\n"	// Get GPS data in NMEA format

// Network commands
#define AT_NW_SET_GPRS			"AT+CGATT=1\r\n"	// Attach from GPRS Service
#define AT_NW_SET_TCPMODE		"AT+CIPMODE=0\r\n"	// Select TCPIP Application Mode
#define AT_NW_SET_SINGLEIP		"AT+CIPMUX=0\r\n"	// Set Single IP connection
#define AT_NW_SET_CONNECTION	"AT+CIICR\r\n"		// Bring up wireless connection with GPRS
#define AT_NW_GET_LOCALIP		"AT+CIFSR\r\n"		// Get Local IP Address
#define AT_NW_GET_RSSI			"AT+CSQ\r\n"		// Get signal strength in dBm

// Network APNs
#define AT_NW_SET_APNVIVO		"AT+CSTT=\"zap.vivo.com.br\",\"vivo\",\"vivo\"\r\n"
#define AT_NW_SET_APNTIM		"AT+CSTT=\"timbrasil.br\",\"tim\",\"tim\"\r\n"
#define AT_NW_SET_APNCLARO		"AT+CSTT=\"claro.com.br\",\"claro\",\"claro\"\r\n"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
int SendCommandSimRadio( char *command, char *response );
int InitSimRadio( void );
int JoinSimRadioNetwork( void );
void GetGpsData( struct sensorData *data );
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// 1 = MODO LP OFF
// 2 = MODO LP ON
uint8_t uiGVLowPowerFlag = 0x01;
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
  /* Infinite loop */
  for(;;)
  {
	uint32_t flags = osThreadFlagsGet();
	if ( flags != 0x00 )
	{
		if ( flags == 0x01 )
		{

			InitSimRadio(); // = int ? capturar erro
			InitImu();


			/* Turn off low-power mode */
			uiGVLowPowerFlag = 0x01;

			/* Turn off system LEDs */
			HAL_GPIO_WritePin(LD_SIM_GPIO_Port, LD_SIM_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LD_GPS_GPIO_Port, LD_GPS_Pin, GPIO_PIN_RESET);

		}
		else if ( flags == 0x02 )
		{
			/* Turn on low-power mode */
			uiGVLowPowerFlag = 0x02;
			LowPowerModeImu();
		}
		flags = osThreadFlagsClear( flags );
	}
	else
	{
		/* Should not change system operation mode */
		if ( uiGVLowPowerFlag == 0x02 )
		{
			//printf("Teste\n");
			/* Blink LEDs to show that the system is on low-power mode */
			HAL_GPIO_TogglePin(LD_GPS_GPIO_Port, LD_GPS_Pin);
			HAL_GPIO_TogglePin(LD_SIM_GPIO_Port, LD_SIM_Pin);
		}
	    osDelay(1000);
	}
  }
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
			  int ret = JoinSimRadioNetwork();
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
	osTimerStart( SensorDataTimerHandle, 10000U );
  /* Infinite loop */
  for(;;)
  {
	  if ( uiGVLowPowerFlag == 0x01 )
	  {
		  /* Low-power mode is OFF */
		  uint32_t flags = osThreadFlagsGet();
		  if ( flags == 0x00000001 )
		  {
			  /* 10 seconds timer has been triggered */
			  ///// TO DO, DEFINIR TAMANHO EM BYTES DA STRUCT SENSORDATA
			  memset( &xGVDataPacket, 0, sizeof(xGVDataPacket) );

			  GetGpsData( &xGVDataPacket );
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

			  /* Tells TaskNwSendData that there is new data to be sent */
			  osThreadFlagsSet( TaskNwSendDataHandle, 0x00000001 );

			  flags = osThreadFlagsClear( flags );
		  }
	  }
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
	  if ( uiGVLowPowerFlag == 0x01 )
	  {

	  }
	  osDelay(100);
  }
  /* USER CODE END FuncNwSendData */
}

/* CallbackTimer function */
void CallbackTimer(void *argument)
{
  /* USER CODE BEGIN CallbackTimer */
	uint32_t ret = osThreadFlagsSet( TaskSensorDataHandle, 0x00000001 );
	if ( (ret >> 7) & 1 )
	{
		/* If bit 8 is set, there's an error on osThreadFlagsSet execution */
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	}
	else
	{
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	}
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

/*
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint32_t teste2;
	if (htim == &htim16)
	{
		teste2 = osThreadFlagsSet( TaskSensorDataHandle, 0x00000001 );
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	}
}*/


int SendCommandSimRadio( char *command, char *response )
{
	uint16_t temp_byte;
	uint16_t prev_byte;
	uint32_t timeout = 400000; // Approx. 10 seconds timeout
	uint8_t i = 0;
	uint8_t amble = 0;

	HAL_UART_Transmit( &huart1, (uint8_t *)command, strlen(command), 1000 );

	while ( 1 && (--timeout > 0) )
	{
		// Read byte from UART
		if ( HAL_UART_Receive( &huart1, &temp_byte, 1, 0 ) == HAL_OK )
		{
			response[i] = temp_byte;
			i++;
		}

		// Check for preamble or postamble
		if (prev_byte == '\r' && temp_byte == '\n')
		{
			if (amble == 0)
			{
				// Preamble \r\n (0x0D 0x0A)
				amble = 1;
			}
			else if (amble == 1)
			{
				// Postamble \r\n (0x0D 0x0A)
				break;
			}
		}
		prev_byte = temp_byte;
	}

	if ( !strcmp(response, "\r\nERROR\r\n") || timeout == 0 )
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int InitSimRadio( void )
{
	char cmd_resp[25] = "\0";
	int ret;

	/*** SIM initialization ***/
	ret = SendCommandSimRadio( AT_SIM_SET_ECHOOFF, cmd_resp );
	if ( strcmp(cmd_resp, "ATE\r\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_SIM_SET_FULLFUNC, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_SIM_GET_CPIN, cmd_resp );
	if ( strcmp(cmd_resp, "\r\n+CPIN: READY\r\n") || ret != 1 )
	{
		return -1;
	}

	/*** Network initialization ***/
	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_NW_SET_GPRS, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_NW_SET_TCPMODE, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_NW_SET_SINGLEIP, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	/*** GPS initialization ***/
	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_GPS_SET_PWRON, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_GPS_SET_HOTMODE, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	return 1;
}

int JoinSimRadioNetwork( void )
{
	char response[25] = "\0";

	SendCommandSimRadio( AT_NW_SET_APNCLARO, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		return -1;
	}

	memset(response, 0, 25);
	SendCommandSimRadio( AT_NW_SET_CONNECTION, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		return -1;
	}

	return 1;
}


void GetGpsData( struct sensorData *data )
{
	char response[128] = "\0";
	char comma[2] = ",";

	int ret;
	ret = SendCommandSimRadio( AT_GPS_GET_DATA, response );
	char *ptr = strtok(response[14], comma);

	data->utcTime = *ptr;
	ptr = strtok(NULL, comma);
	data->gpsStatus = *ptr; 		// possui byte 0 no final, talvez seja preciso remover
	ptr = strtok(NULL, comma);
	data->latitude = *ptr;
	ptr = strtok(NULL, comma);
	data->nsIndicator = *ptr;
	ptr = strtok(NULL, comma);
	data->longitude = *ptr;
	ptr = strtok(NULL, comma);
	data->ewIndicator = *ptr;
	ptr = strtok(NULL, comma);
	data->speedKnots = *ptr;
	ptr = strtok(NULL, comma);
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
