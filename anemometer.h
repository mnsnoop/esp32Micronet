#ifndef _ANEMOMETER_H
#define _ANEMOMETER_H

#include "nmea2kbridge.h"
#include "runningAngle.h"
#include "RunningAverage.h"
#include "adc_cal.h" //to be accurate we need accurate ADC readings. the ESP32's ADC's are not calibrated correctly so we use a lookup table to correct them. Check out https://github.com/e-tinkers/esp32-adc-calibrate for more info.

//Collect data from a 5 wire raymarine anemometer.

#define Power12V 4
#define Power8V 15
#define Power5V 2
#define SpeedIO 13
#define SineADC 14
#define CosineADC 27

#define WindSpeedDebounceMS 5
#define WindSpeedAvgSamples 10 //Number of running average sample size. Larger is more smoothing.
#define VoltagesAvgSamples 10 //Number of running average sample size. Larger is more smoothing.
#define WindAngleSmoothing 0.4 //weigth The smoothing coefficient, α, is the weight of the current input in the average. It is called “weight” within the library, and should be set to a value between 0.001 and 1. The larger the weight, the weaker the smoothing. A weight α = 1 provides no smoothing at all, as the filter's output is a just a copy of its input.

class Anemometer
{
	TaskHandle_t tAnemometer;

	RunningAverage *raAvg12V;
	RunningAverage *raAvg8V;
	RunningAverage *raAvg5V;
	RunningAverage *raAvgWindSpeed;
	runningAngle *raAvgWindAngle;

	bool bFirstRep = true;
	
public:
	Anemometer();
	~Anemometer();

	void Start();
	void Rep();
	friend void tfAnemometer(void *p);
};

extern Anemometer Ameter;
#endif // _ANEMOMETER_H
