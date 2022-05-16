/*
 * CompassDecoder.cpp
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#include "BoardConfig.h"
#include "Globals.h"
#include "NavCompass.h"
#include "LSM303DLHCDriver.h"

NavCompass::NavCompass() :
		headingIndex(0), navCompassDetected(false), navCompassDriver(nullptr)
{
	for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
	{
		headingHistory[i] = 0.0f;
	}
}

NavCompass::~NavCompass()
{
}

bool NavCompass::Init()
{
	navCompassDetected = false;

	navCompassDriver = new LSM303DLHCDriver();

	if (!navCompassDriver->Init())
	{
		delete navCompassDriver;
		return false;
	}

	navCompassDetected = true;
	return true;
}

string NavCompass::GetDeviceName()
{
	if (navCompassDetected)
	{
		return navCompassDriver->GetDeviceName();
	}

	return string("");
}

float NavCompass::GetHeading()
{
	float mx, my, mz;
	float ax, ay, az;
	// Get Acceleration and Magnetic data from LSM303
	navCompassDriver->GetAcceleration(&ax, &ay, &az);
//Serial.printf("ax %f ay %f az %f\n", ax, ay, az);
	navCompassDriver->GetMagneticField(&mx, &my, &mz);
//Serial.printf("1 mx %f my %f mz %f\n", mx, my, mz);

#define WITH_LP_FILTER
#ifdef WITH_LP_FILTER
	// Low-Pass filter accelerometer
	fXa = ax * alpha + fXa * (1.0f - alpha);
	fYa = ay * alpha + fYa * (1.0f - alpha);
	fZa = az * alpha + fZa * (1.0f - alpha);
	ax = fXa;
	ay = fYa;
	az = fZa;
	// Low-Pass filter magnetometer
	fXm = mx * alpha + fXm * (1.0f - alpha);
	fYm = my * alpha + fYm * (1.0f - alpha);
	fZm = mz * alpha + fZm * (1.0f - alpha);
	mx = fXm;
	my = fYm;
	mz = fZm;
#endif
//Serial.printf("2 mx %f my %f mz %f\n", mx, my, mz);

// Adopted from:
// https://github.com/pololu/lsm303-arduino

	// subtract offset (average of min and max) from magnetometer readings
  mx -= -28.5f;
  my -= -112.0f;
  mz -= +24.0f;
//Serial.printf("3 mx %f my %f mz %f\n", mx, my, mz);

	vector<float> from = {1.0f, 0.0f, 0.0f}; // x axis is reference direction
	a.x = ax;
	a.y = ay;
	a.z = az;
	m.x = mx;
	m.y = my;
	m.z = mz;

	// normalize
	vector_normalize(&a);
	vector_normalize(&m);

	// compute E and N
	vector<float> E;
	vector<float> N;
	// D X M = E, cross acceleration vector Down with M (magnetic north + inclination) to produce "East"
	vector_cross(&m, &a, &E);
	vector_normalize(&E);
	// E X D = N, cross "East" with "Down" to produce "North" (parallel to the ground)
	vector_cross(&a, &E, &N);
	vector_normalize(&N);

	// compute heading
	heading = atan2(vector_dot(&E, &from), vector_dot(&N, &from)) * 180.0f / PI;

	if (heading < 0)
		heading += 360;
//Serial.print("Heading: "); Serial.println(heading);
	return heading;
}

void NavCompass::vector_normalize(vector<float> *a)
{
	float mag = sqrt(vector_dot(a, a));
	a->x /= mag;
	a->y /= mag;
	a->z /= mag;
}

void NavCompass::GetMagneticField(float *magX, float *magY, float *magZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetMagneticField(magX, magY, magZ);
	}
}

void NavCompass::GetAcceleration(float *accX, float *accY, float *accZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetAcceleration(accX, accY, accZ);
	}
}
