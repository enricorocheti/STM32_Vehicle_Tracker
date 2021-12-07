#include "sim808.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include <stdio.h>

int SendCommandSimRadio_NEW( char *command, char *response, uint16_t size_cmd, uint16_t size_rsp )
{
	int ret;

	HAL_UART_Transmit_IT( &huart1, (uint8_t *)command, size_cmd );

	if ( HAL_UART_Receive_IT( &huart1, (uint8_t *) response, size_rsp ) == HAL_OK )
	{
		ret = 1;
	}
	else
	{
		ret = -1;
	}

	return ret;
}



int SendCommandSimRadio( const char *command, char *response )
{
	uint8_t temp_byte = 0;
	uint8_t prev_byte = 0;
	uint8_t i = 0;
	uint8_t amble = 0;
	uint16_t error = 0;

	HAL_UART_Transmit( &huart1, (uint8_t *)command, strlen(command), 100 );

	while ( 1 )
	{
		// Read byte from UART
		if ( HAL_UART_Receive( &huart1, &temp_byte, 1, 50 ) == HAL_OK )
		{
			response[i] = temp_byte;
			i++;
		}
		else
		{
			error++;
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

		if ( error == 50 )
		{
			break;
		}
	}

	if ( !strcmp( response, "\r\nERROR\r\n" ) || error == 100 )
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
	char cmd_resp[25];
	int ret = 0;
	int sim_ret;

	/*** SIM initialization ***/
	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_SIM_SET_FULLFUNC, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b1;
	}

	/*memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_SIM_GET_CPIN, cmd_resp );
	if ( strcmp(cmd_resp, "\r\n+CPIN: READY\r\n") || sim_ret != 1 )
	{
		ret |= 0b10;
	}*/

	/*** Network initialization ***/
	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_NW_SET_GPRS, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b100;
	}

	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_NW_SET_TCPMODE, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b1000;
	}

	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_NW_SET_SINGLEIP, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b10000;
	}

	/*** GPS initialization ***/
	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_GPS_SET_PWRON, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b100000;
	}

	memset(cmd_resp, 0, 25);
	sim_ret = SendCommandSimRadio( AT_GPS_SET_HOTMODE, cmd_resp );
	if ( strcmp(cmd_resp, "\r\nOK\r\n") || sim_ret != 1 )
	{
		ret |= 0b1000000;
	}

	return ret;
}


int JoinSimRadioNetwork( void )
{
	int ret = 0;
	char response[25] = "\0";

	SendCommandSimRadio( AT_NW_SET_APNCLARO, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		ret |= 0b1;
	}

	memset( response, 0, 25 );
	SendCommandSimRadio( AT_NW_SET_CONNECTION, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		ret |= 0b10;
	}

	return ret;
}


int SendNetworkData( struct sensorData *data )
{
	char response[64] = "\0";
	char dataCommand[256] = "\0";
	int dataLength;

	SendCommandSimRadio( AT_TCP_START, response );
	if ( strcmp(response, "\r\nOK\r\nCONNECT OK\r\n") )
	{
		return -2;
	}

	sprintf( dataCommand,
			 "GET /update?api_key=T8D5CZES6W7RV89G"
			 "&field1=%f&field2=%f&field3=%f&field4=%f"
			 "&field5=%f&field6=%f&field7=%f&field8=%f",
			 data->latitude, data->longitude, data->accelX, data->accelY,
			 data->accelZ, data->gyroX, data->gyroY, data->gyroZ );
	dataLength = strlen( dataCommand );

	/* Post amble eh diferente, recebe apenas 0D 0A 3E 20 */
	SendCommandSimRadio( AT_TCP_SEND_BYTES(dataLength), response );
	if ( ! strcmp(response, "\r\n> ") )
	{
		memset( response, 0, 64 );
		SendCommandSimRadio( AT_TCP_SEND_BYTES(dataLength), response );
		if( strstr(response, "CME ERROR") != NULL || strstr(response, "SEND FAIL") != NULL )
		{
		    /* Datasheet p224: * If connection is not established or module is disconnected */
			return -1;
		}
	}

	return 1;
}


int GetGpsData( struct sensorData *data )
{
	char response[128] = "\0";
	char comma[2] = ",";

	int ret;
	ret = SendCommandSimRadio( AT_GPS_GET_DATA, response );
	char *ptr = strtok( &response[14], comma );

	data->utcTime = strtof(ptr, NULL);
	ptr = strtok(NULL, comma);
	data->gpsStatus = *ptr;
	ptr = strtok(NULL, comma);
	data->latitude = strtof(ptr, NULL);
	ptr = strtok(NULL, comma);
	data->nsIndicator = *ptr;
	ptr = strtok(NULL, comma);
	data->longitude = strtof(ptr, NULL);
	ptr = strtok(NULL, comma);
	data->ewIndicator = *ptr;
	ptr = strtok(NULL, comma);
	data->speedKnots = strtof(ptr, NULL);
	ptr = strtok(NULL, comma);
	/* Cut off the COG field from NMEA because it's unnecessary */
	ptr = strtok(NULL, comma);
	data->date = strtod(ptr, NULL);

	return ret;
}


int LowPowerModeSimRadio( void )
{
	int ret = 0;
	char response[25] = "\0";

	SendCommandSimRadio( AT_SIM_SET_MINFUNC, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		ret |= 0b1;
	}

	memset(response, 0, 25);
	SendCommandSimRadio( AT_GPS_SET_PWROFF, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		ret |= 0b10;
	}

	return ret;
}

int PowerOnSimRadio( void )
{
	char cmd_resp[25];
	int ret;

	/* Set SIM808 EVB-V3.2 D9 pin high for one second to turn on the radio */
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_SET);
	osDelay(1000);
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_RESET);
	osDelay(1000);

	/* Disable command echo */
	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_SIM_SET_ECHOOFF, cmd_resp );
	if ( strcmp(cmd_resp, "ATE0\r\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	return 1;
}
