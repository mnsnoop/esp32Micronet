#ifndef _PADDLEWHEEL_H
#define _PADDLEWHEEL_H

#include "nmea2kbridge.h"
#include "RunningAverage.h"

//Collect data from an airmar st850-p17 speed/temp transducer. (we only get speed)

#define WaterSpeedIO 33

#define WaterSpeedDebounceMS 10
#define WaterSpeedAvgSamples 10 //Number of running average sample size. Larger is more smoothing.

class Paddlewheel
{
	RunningAverage *raAvgWaterSpeed;

	bool bFirstRep = true;
	
public:
	Paddlewheel();
	~Paddlewheel();

	void Start();
	friend void cbWaterSpeedTimer(void *v);
};

extern Paddlewheel Pwheel;
#endif // _PADDLEWHEEL_H
