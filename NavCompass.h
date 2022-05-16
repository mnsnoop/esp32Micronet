/*
 * CompassDecoder.h
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#ifndef NAVCOMPASS_H_
#define NAVCOMPASS_H_

#include "NavCompassDriver.h"

#include <stdint.h>
#include <string>

using string = std::string;

#define HEADING_HISTORY_LENGTH 8

#define LSM303DLH_MAG_ADDR  0x1E
#define LSM303DLH_ACC_ADDR  0x19

class NavCompass
{
public:
	NavCompass();
	virtual ~NavCompass();

	bool Init();
	string GetDeviceName();
	float GetHeading();
	void GetMagneticField(float *magX, float *magY, float *magZ);
	void GetAcceleration(float *accX, float* accY, float *accZ);

	template <typename T> struct vector
	{
	  T x, y, z;
	};
	vector<float> a; // accelerometer readings
	vector<float> m; // magnetometer readings

	// vector functions
	template <typename Ta, typename Tb, typename To> static void vector_cross(const vector<Ta> *a, const vector<Tb> *b, vector<To> *out);
	template <typename Ta, typename Tb> static float vector_dot(const vector<Ta> *a, const vector<Tb> *b);
	static void vector_normalize(vector<float> *a);

private:
	float headingHistory[HEADING_HISTORY_LENGTH];
	uint32_t headingIndex;
	bool navCompassDetected;
	NavCompassDriver *navCompassDriver;
	float heading;
	float magX, magY, magZ;
	float accX, accY, accZ;
	float alpha = 0.15f;
	float fXa = 0.0f, fYa = 0.0f, fZa = 0.0f, fXm = 0.0f, fYm = 0.0f, fZm = 0.0f;
};

template <typename Ta, typename Tb, typename To> void NavCompass::vector_cross(const vector<Ta> *a, const vector<Tb> *b, vector<To> *out)
{
	out->x = (a->y * b->z) - (a->z * b->y);
	out->y = (a->z * b->x) - (a->x * b->z);
	out->z = (a->x * b->y) - (a->y * b->x);
}

template <typename Ta, typename Tb> float NavCompass::vector_dot(const vector<Ta> *a, const vector<Tb> *b)
{
	return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

#endif /* NAVCOMPASS_H_ */
