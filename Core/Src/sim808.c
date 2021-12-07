#include "sim808.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include <stdio.h>

uint8_t uart1_rx_buffer[512] = { '\0' };
uint8_t uart1_rx_byte = '\0';
int16_t uart1_rx_index = 0;

void HAL_UART_RxCpltCallback( UART_HandleTypeDef *huart )
{
	if (huart->Instance == USART1)
	{
		if (uart1_rx_index > (512 - 1))
		{
			uart1_rx_index = 0;
		}
		uart1_rx_buffer[uart1_rx_index++] = uart1_rx_byte;
		HAL_UART_Receive_IT( &huart1, &uart1_rx_byte, 1 );
	}
}

void ClearRxBuffer( void )
{
	uart1_rx_index = 0;
	memset(uart1_rx_buffer, 0, sizeof(uart1_rx_buffer));
}

int8_t SendAtCommand( const char *cmd, const char *ack, int16_t timeout )
{

	uart1_rx_index = 0;
	uart1_rx_buffer[0] = '\0';
	int8_t ret = 1;

	HAL_UART_Receive_IT( &huart1, &uart1_rx_byte, 1 );
	if( HAL_OK != HAL_UART_Transmit( &huart1, (uint8_t *)cmd, strlen(cmd), 100 ) )
	{
		return ret;
	}

	while ( timeout > 0 && ret == 1 )
	{
		if ( strstr( (char *)uart1_rx_buffer, ack ) )
		{
			ret = 0;
		}
		osDelay(100);
		timeout -= 100;
	}
	ClearRxBuffer();
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
	int ret = 0;
	int sim_ret;

	/*** SIM initialization ***/
	sim_ret = SendAtCommand( AT_SIM_SET_FULLFUNC, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b1;
	}

	/*sim_ret = SendAtCommand( AT_SIM_GET_CPIN, "READY", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b10;
	}*/

	/*** Network initialization ***/
	sim_ret = SendAtCommand( AT_NW_SET_GPRS, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b100;
	}

	sim_ret = SendAtCommand( AT_NW_SET_TCPMODE, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b1000;
	}

	sim_ret = SendAtCommand( AT_NW_SET_SINGLEIP, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b10000;
	}

	/*** GPS initialization ***/
	sim_ret = SendAtCommand( AT_GPS_SET_PWRON, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b100000;
	}

	sim_ret = SendAtCommand( AT_GPS_SET_HOTMODE, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b1000000;
	}

	return ret;
}


int JoinSimRadioNetwork( void )
{
	int ret = 0;
	int sim_ret = 0;

	sim_ret = SendAtCommand( AT_NW_SET_APNCLARO, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b1;
	}

	sim_ret = SendAtCommand( AT_NW_SET_CONNECTION, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b10;
	}

	return ret;
}


int SendNetworkData( struct sensorData *data )
{
	char dataCommand[256] = "\0";
	int dataLength;
	int sim_ret = 0;

	sim_ret = SendAtCommand( AT_TCP_START, "CONNECT OK", 5000 );
	if ( sim_ret == 1 )
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
	sim_ret = SendAtCommand( AT_TCP_SEND_BYTES(dataLength), ">", 5000 );
	if ( sim_ret == 1 )
	{
		sim_ret = SendAtCommand( AT_TCP_SEND_BYTES(dataLength), "SEND OK", 5000 );
		if ( sim_ret == 1 )
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
	int sim_ret = 0;

	sim_ret = SendAtCommand( AT_SIM_SET_MINFUNC, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b1;
	}

	sim_ret = SendAtCommand( AT_GPS_SET_PWROFF, "OK", 1000 );
	if ( sim_ret == 1 )
	{
		ret |= 0b10;
	}

	return ret;
}

int PowerOnSimRadio( void )
{
	int ret;

	/* Set SIM808 EVB-V3.2 D9 pin high for one second to turn on the radio */
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_SET);
	osDelay(1000);
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_RESET);
	osDelay(1000);

	/* Disable command echo */
	ret = SendAtCommand( AT_SIM_SET_ECHOOFF, "OK", 1000 );

	return ret;
}
