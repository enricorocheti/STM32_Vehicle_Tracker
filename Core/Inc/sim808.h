/*
 * File name: 			sim808.h
 * File description: 	Functions prototypes to handle the SIM808 radio
 * Author name: 		Enrico Oliveira Rocheti
 * Revision date: 		07/12/2021
*/


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


/*
 * Method name: 		SendAtCommand
 * Method description: 	Send AT command to radio and check if ack is in its response
 * Input params: 		command: 		AT command to be sent
 * 						ack: 			expected response
 * 						timeout:		timeout in ms if the radio do not answer
 * Output params: 		return 0:		ack was found on radio response
 * 						other than 0: 	ERROR
*/
int SendAtCommand( const char *command, const char *ack, uint16_t timeout );


/*
 * Method name: 		ClearRxBuffer
 * Method description: 	Clear global variable Rx Buffer
 * Input params: 		n/a
 * Output params: 		n/a
*/
void ClearRxBuffer( void );


/*
 * Method name: 		InitSimRadio
 * Method description: 	Configure radio initializtion with AT commands
 * Input params: 		n/a
 * Output params: 		return 0:		OK
 * 						other than 0: 	ERROR
*/
int InitSimRadio( void );


/*
 * Method name: 		JoinSimRadioNetwork
 * Method description: 	Register on radio configured network
 * Input params: 		n/a
 * Output params: 		return 0:		OK
 * 						other than 0: 	ERROR
*/
int JoinSimRadioNetwork( void );


/*
 * Method name: 		SendNetworkData
 * Method description: 	Send data through radio network
 * Input params: 		data:			struct containing data to be sent
 * Output params: 		return 0:		OK
 * 						return -1: 		TCP ERROR
 * 						return -2:		Network ERROR
*/
int SendNetworkData( struct sensorData *data );


/*
 * Method name: 		ReadGpsData
 * Method description: 	Read raw GPS data from radio
 * Input params: 		command:		AT command to be sent
 * 						response:		char pointer to write the radio response
 * Output params: 		return 0: 		OK
 * 						return 1: 		ERROR
*/
int ReadGpsData( const char *command, char *response );


/*
 * Method name: 		GetGpsData
 * Method description: 	Get GPS data from radio
 * Input params: 		data:			struct to receive the GPS's formatted data
 * Output params: 		return 0: 		OK
 * 						return 1: 		ERROR
*/
int GetGpsData( struct sensorData *data );


/*
 * Method name: 		LowPowerModeSimRadio
 * Method description: 	Set radio to low-power mode
 * Input params: 		n/a
 * Output params: 		return 0:		OK
 * 						other than 0: 	ERROR
*/
int LowPowerModeSimRadio( void );


/*
 * Method name: 		PowerOnSimRadio
 * Method description: 	Turn radio on using D9 pin, disable AT command echo
 * Input params: 		n/a
 * Output params: 		return 0:		OK
 * 						other than 0: 	ERROR
*/
int PowerOnSimRadio( void );


#endif
