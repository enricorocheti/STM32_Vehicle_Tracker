/*
 * File name: 			global.h
 * File description: 	Contains definitions that's used by multiple system files
 * Author name: 		Enrico Oliveira Rocheti
 * Revision date: 		07/12/2021
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif


struct __attribute__((packed)) sensorData
{
	/* GPS */
	float utcTime;			// hhmmss.sss
	uint8_t gpsStatus;			// A valid, V not valid
	float latitude;
	uint8_t nsIndicator;
	float longitude;
	uint8_t ewIndicator;
	float speedKnots;
	uint32_t date;			// ddmmyy

	/* IMU */
	float accelX;
	float accelY;
	float accelZ;
	float gyroX;
	float gyroY;
	float gyroZ;

};

#endif
