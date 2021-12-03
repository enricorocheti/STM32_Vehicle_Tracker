#include "sim808.h"
#include "main.h"


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



int SendCommandSimRadio( char *command, char *response )
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


int GetGpsData( struct sensorData *data )
{
	char response[128] = "\0";
	char comma[2] = ",";

	int ret;
	ret = SendCommandSimRadio( AT_GPS_GET_DATA, response );
	char *ptr = strtok( &response[14], comma );

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

	return ret;
}


int LowPowerModeSimRadio( void )
{
	char response[25] = "\0";

	SendCommandSimRadio( AT_SIM_SET_MINFUNC, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		return -1;
	}

	memset(response, 0, 25);
	SendCommandSimRadio( AT_GPS_SET_PWROFF, response );
	if ( strcmp(response, "\r\nOK\r\n") )
	{
		return -1;
	}

	return 1;
}

int PowerOnSimRadio( void )
{
	char cmd_resp[25];
	int ret;

	/* Set SIM808 EVB-V3.2 D9 pin high for one second to turn on the radio */
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_SET);
	osDelay(1000);
	HAL_GPIO_WritePin( POWER_SIM_GPIO_Port, POWER_SIM_Pin, GPIO_PIN_RESET);
	osDelay(100);

	/* Disable command echo */
	memset(cmd_resp, 0, 25);
	ret = SendCommandSimRadio( AT_SIM_SET_ECHOOFF, cmd_resp );
	if ( strcmp(cmd_resp, "ATE0\r\r\nOK\r\n") || ret != 1 )
	{
		return -1;
	}

	return 1;
}
