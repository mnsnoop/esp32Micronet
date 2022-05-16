#include "paddlewheel.h"

volatile int iWaterSpeedCount = 0;
volatile float fWaterSpeed = 0;

TimerHandle_t swWaterSpeedTimer;
portMUX_TYPE mxWaterSpeedTimer = portMUX_INITIALIZER_UNLOCKED;

Paddlewheel Pwheel;

void IRAM_ATTR WaterSpeedInt()
{
//	bool bWaterSpeed = digitalRead(WaterSpeedIO);

	portENTER_CRITICAL_ISR(&mxWaterSpeedTimer);
	iWaterSpeedCount++;
	portEXIT_CRITICAL_ISR(&mxWaterSpeedTimer);
}

void IRAM_ATTR cbWaterSpeedTimer(void *v)
{
	portENTER_CRITICAL_ISR(&mxWaterSpeedTimer);

	//4.72 pulses per NM.
	
	fWaterSpeed = iWaterSpeedCount * (double)0.21186f * 4.0f; //using doubles because esp32's can't do floating point in interupts.
	iWaterSpeedCount = 0;

	Pwheel.raAvgWaterSpeed->add(fWaterSpeed); //this is in knots.

	if (Pwheel.bFirstRep)
	{
		for (int i = 0; i < 50; i++)
			Pwheel.raAvgWaterSpeed->add(fWaterSpeed);

		Pwheel.bFirstRep = false;
	}

	
	float fAvgWaterSpeed = Pwheel.raAvgWaterSpeed->getAverage();

	portEXIT_CRITICAL_ISR(&mxWaterSpeedTimer);

	if (fAvgWaterSpeed > 0.1f)
	{
		mNet.SetSpeed(fAvgWaterSpeed);
		N2kBridge.SendWaterSpeed(fAvgWaterSpeed * 0.514f);
	}

//	Serial.print("speed ");
//	Serial.print(fAvgWaterSpeed);
//	Serial.print("knots\n");
}

Paddlewheel::Paddlewheel()
{
}

Paddlewheel::~Paddlewheel()
{
	print(LL_INFO, "Stopping the paddlewheel transducer monitor.\n");

	detachInterrupt(WaterSpeedIO);
	xTimerStop(swWaterSpeedTimer, portMAX_DELAY);
	xTimerDelete(swWaterSpeedTimer, portMAX_DELAY);
	
	delete(raAvgWaterSpeed);
}

void Paddlewheel::Start()
{
	print(LL_INFO, "Starting the paddlewheel transducer monitor.\n");

	pinMode(WaterSpeedIO, INPUT);

	raAvgWaterSpeed = new RunningAverage(WaterSpeedAvgSamples);

	swWaterSpeedTimer = xTimerCreate("Paddlewheel timer", 1000, pdTRUE, (void *) 0, cbWaterSpeedTimer);
	xTimerStart(swWaterSpeedTimer, portMAX_DELAY);

	attachInterrupt(WaterSpeedIO, WaterSpeedInt, FALLING);
}
