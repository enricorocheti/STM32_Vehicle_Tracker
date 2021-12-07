#ifndef __SIM808_H__
#define __SIM808_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
#include "global.h"
#include "usart.h"


// General commands
#define AT_SIM_SET_FULLFUNC		"AT+CFUN=1\r\n"		// Set Phone to Full functionality
#define AT_SIM_SET_MINFUNC		"AT+CFUN=0\r\n"		// Set Phone to Minimum functionality
#define AT_SIM_GET_CPIN			"AT+CPIN?\r\n"		// Check if SIM card is inserted
#define AT_SIM_SET_ECHOOFF		"ATE0\r\n"			// Disable commands echo

#define AT_TEST 				"AT+CFUN?\r\n"

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

#define AT_TCP_START			"AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n"
#define AT_TCP_SEND_BYTES(arg)  "AT+CIPSEND="#arg"\r\n"

// Network APNs
#define AT_NW_SET_APNVIVO		"AT+CSTT=\"zap.vivo.com.br\",\"vivo\",\"vivo\"\r\n"
#define AT_NW_SET_APNTIM		"AT+CSTT=\"timbrasil.br\",\"tim\",\"tim\"\r\n"
#define AT_NW_SET_APNCLARO		"AT+CSTT=\"datelo.com.br\",\"claro\",\"claro\"\r\n"

int SendCommandSimRadio( const char *command, char *response );

int InitSimRadio( void );

int JoinSimRadioNetwork( void );

int SendNetworkData( struct sensorData *data );

int GetGpsData( struct sensorData *data );

int LowPowerModeSimRadio( void );

int PowerOnSimRadio( void );

int SendCommandSimRadio_NEW( char *command, char *response, uint16_t size_cmd, uint16_t size_rsp );

#endif
