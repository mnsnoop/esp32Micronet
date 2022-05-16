#ifndef _NMEA2KBRIDGE_H
#define _NMEA2KBRIDGE_H

#include "micronet.h"

inline double MeterstoFeet(double m)
{
	return m * 3.2808398950131;	
}

inline double FeettoMeters(double f)
{
	return f / 3.2808398950131;	
}

inline double MeterstoNM(double m)
{
	return m * 0.00053996;
}

class NMEA2kBridge
{
	TaskHandle_t tNMEA2K;
	
public:
	NMEA2kBridge();
	~NMEA2kBridge();

	void Start();
	void SendAppWindAngleSpeed(double dAngle, double dSpeed);
	void SendBatteryVoltage(double dVolts);
	void SendWaterSpeed(double dSpeed);
};

extern NMEA2kBridge N2kBridge;

#endif // _NMEA2KBRIDGE_H
