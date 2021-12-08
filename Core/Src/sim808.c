/*
 * File name: 			sim808.c
 * File description: 	Functions to handle the SIM808 radio
 * Author name: 		Enrico Oliveira Rocheti
 * Revision date: 		07/12/2021
*/


#include "sim808.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include <stdio.h>


uint8_t uiRxBuffer[512] = { '\0' };
uint8_t uiRxByte = '\0';
int16_t iRxIndex = 0;


void HAL_UART_RxCpltCallback( UART_HandleTypeDef *huart )
{
	if ( huart->Instance == USART1 )
	{
		if ( iRxIndex > (512 - 1) )
		{
			iRxIndex = 0;
		}
		uiRxBuffer[iRxIndex++] = uiRxByte;
		HAL_UART_Receive_IT( &huart1, &uiRxByte, 1 );
	}
}


void ClearRxBuffer( void )
{
	iRxIndex = 0;
	memset( uiRxBuffer, 0, sizeof(uiRxBuffer) );
}


int SendAtCommand( const char *command, const char *ack, uint16_t timeout )
{
	/* Function based on the following references:
	 *
	 * https://github.com/leech001/SIM800MQTT/blob/master/src/MQTTSim800.c
	 * https://gist.github.com/hidsts/40a9323f164d7276e5672dd2544f7719
	 *
	 */

	iRxIndex = 0;
	uiRxBuffer[0] = '\0';
	int iRet = 1;

	HAL_UART_Receive_IT( &huart1, &uiRxByte, 1 );
	if( HAL_OK != HAL_UART_Transmit( &huart1, (uint8_t *)command, strlen(command), 100 ) )
	{
		return iRet;
	}

	while ( timeout > 0 && iRet == 1 )
	{
		if ( strstr( (char *)uiRxBuffer, ack ) )
		{
			iRet = 0;
		}
		osDelay(100);
		timeout -= 100;
	}
	ClearRxBuffer();
	return iRet;
}


int ReadGpsData( const char *command, char *response )
{
	uint8_t uiTempByte = 0;
	uint8_t uiPrevByte = 0;
	uint8_t i = 0;
	uint8_t uiAmble = 0;
	uint16_t uiError = 0;

	HAL_UART_Transmit( &huart1, (uint8_t *)command, strlen(command), 100 );

	while ( 1 )
	{
		/* Read byte from UART */
		if ( HAL_UART_Receive( &huart1, &uiTempByte, 1, 50 ) == HAL_OK )
		{
			response[i] = uiTempByte;
			i++;
		}
		else
		{
			uiError++;
		}

		/* Check for preuiAmble or postuiAmble */
		if (uiPrevByte == '\r' && uiTempByte == '\n')
		{
			if (uiAmble == 0)
			{
				/* PreuiAmble \r\n (0x0D 0x0A) */
				uiAmble = 1;
			}
			else if (uiAmble == 1)
			{
				/* PostuiAmble \r\n (0x0D 0x0A) */
				break;
			}
		}
		uiPrevByte = uiTempByte;

		if ( uiError == 50 )
		{
			break;
		}
	}

	if ( !strcmp( response, "\r\nERROR\r\n" ) || uiError == 50 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


int InitSimRadio( void )
{
	int iRet = 0;
	int iSimRet;

	/* SIM initialization */
	iSimRet = SendAtCommand( AT_SIM_SET_FULLFUNC, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b1;
	}

	/* Network initialization */
	iSimRet = SendAtCommand( AT_NW_SET_GPRS, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b100;
	}

	iSimRet = SendAtCommand( AT_NW_SET_TCPMODE, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b1000;
	}

	iSimRet = SendAtCommand( AT_NW_SET_SINGLEIP, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b10000;
	}

	/* GPS initialization */
	iSimRet = SendAtCommand( AT_GPS_SET_PWRON, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b100000;
	}

	iSimRet = SendAtCommand( AT_GPS_SET_HOTMODE, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b1000000;
	}

	return iRet;
}


int JoinSimRadioNetwork( void )
{
	int iRet = 0;
	int iSimRet = 0;

	iSimRet = SendAtCommand( AT_NW_SET_APNCLARO, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b1;
	}

	iSimRet = SendAtCommand( AT_NW_SET_CONNECTION, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b10;
	}

	return iRet;
}


int SendNetworkData( struct sensorData *data )
{
	char cDataCommand[256] = "\0";
	int iDataLength;
	int iSimRet = 0;

	iSimRet = SendAtCommand( AT_TCP_START, "CONNECT OK", 5000 );
	if ( iSimRet == 1 )
	{
		return -1;
	}

	sprintf( cDataCommand,
			 "GET /update?api_key=T8D5CZES6W7RV89G&field1=%f&field2=%f&field3=%f&field4=%f&field5=%f&field6=%f&field7=%f&field8=%f",
			 data->latitude, data->longitude, data->accelX, data->accelY,
			 data->accelZ, data->gyroX, data->gyroY, data->gyroZ );
	iDataLength = strlen( cDataCommand );

	iSimRet = SendAtCommand( AT_TCP_SEND_BYTES(iDataLength), ">", 5000 );
	if ( iSimRet == 1 )
	{
		iSimRet = SendAtCommand( cDataCommand, "SEND OK", 5000 );
		if ( iSimRet == 1 )
		{
			return -2;
		}
	}

	return 0;
}


int GetGpsData( struct sensorData *data )
{
	char cResponse[128] = "\0";
	char cComma[2] = ",";

	int iRet;
	iRet = ReadGpsData( AT_GPS_GET_DATA, cResponse );
	char *ptr = strtok( &cResponse[14], cComma );

	data->utcTime = strtof(ptr, NULL);
	ptr = strtok(NULL, cComma);
	data->gpsStatus = *ptr;
	ptr = strtok(NULL, cComma);
	data->latitude = strtof(ptr, NULL);
	ptr = strtok(NULL, cComma);
	data->nsIndicator = *ptr;
	ptr = strtok(NULL, cComma);
	data->longitude = strtof(ptr, NULL);
	ptr = strtok(NULL, cComma);
	data->ewIndicator = *ptr;
	ptr = strtok(NULL, cComma);
	data->speedKnots = strtof(ptr, NULL);
	ptr = strtok(NULL, cComma);
	/* Cut off the COG field from NMEA because it's unnecessary */
	ptr = strtok(NULL, cComma);
	data->date = strtod(ptr, NULL);

	return iRet;
}


int LowPowerModeSimRadio( void )
{
	int iRet = 0;
	int iSimRet = 0;

	iSimRet = SendAtCommand( AT_SIM_SET_MINFUNC, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b1;
	}

	iSimRet = SendAtCommand( AT_GPS_SET_PWROFF, "OK", 1000 );
	if ( iSimRet == 1 )
	{
		iRet |= 0b10;
	}

	return iRet;
}


int PowerOnSimRadio( void )
{
	int iRet;

	/* Set SIM808 EVB-V3.2 D9 pin high for one second to turn on the radio */
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_SET);
	osDelay(1000);
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_RESET);
	osDelay(1000);

	/* Disable command echo */
	iRet = SendAtCommand( AT_SIM_SET_ECHOOFF, "OK", 1000 );

	return iRet;
}
