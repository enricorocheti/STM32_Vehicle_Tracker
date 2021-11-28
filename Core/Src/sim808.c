#include "sim808.h"


int SendCommandSimRadio( char *command, char *response )
{
	uint8_t temp_byte;
	uint8_t prev_byte;
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
