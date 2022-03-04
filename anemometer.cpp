#include "anemometer.h"

volatile int iWindCount = 0;
volatile float fWindSpeed = 0;
bool bWindSpeedIO = false;
unsigned long ulWindSpeedDebounce = 0;
bool bWindSpeedDebounce = false;

TimerHandle_t swWindTimer;
portMUX_TYPE mxWindTimer = portMUX_INITIALIZER_UNLOCKED;

Anemometer Ameter;

void IRAM_ATTR SpeedInt()
{
	unsigned long ulTime = millis();
	bool bSpeed = digitalRead(SpeedIO);
	
	if (abs(ulTime - ulWindSpeedDebounce) > WindSpeedDebounceMS &&
		bWindSpeedDebounce != bSpeed)
	{
		portENTER_CRITICAL_ISR(&mxWindTimer);
		iWindCount++;
		portEXIT_CRITICAL_ISR(&mxWindTimer);

		ulWindSpeedDebounce = ulTime;
		bWindSpeedDebounce = bSpeed;
	}
}

void IRAM_ATTR cbSpeedTimer(void *v)
{
	portENTER_CRITICAL_ISR(&mxWindTimer);
	fWindSpeed = iWindCount * (double)0.08; //using doubles because esp32's can't do floating point in interupts.
	iWindCount = 0;
	portEXIT_CRITICAL_ISR(&mxWindTimer);
}

void tfAnemometer(void *p)
{
	for (;;)
	{
		int iRaw12VPower = ADC_LUT[analogRead(Power12V)],
			iRaw8VPower = ADC_LUT[analogRead(Power8V)],
			iRaw5VPower = ADC_LUT[analogRead(Power5V)],
			iRawSine = ADC_LUT[analogRead(SineADC)],
			iRawCosine = ADC_LUT[analogRead(CosineADC)];

		float 	fPower12Volts = iRaw12VPower / 223.0f,
				fPower8Volts = iRaw8VPower / 471.0f,
				fPower5Volts = iRaw5VPower / 706.0f,
				fSineVolts = iRawSine / 642.0f,
				fCosineVolts = iRawCosine / 642.0f,
				fWindAngle = atan2(fSineVolts - (fPower8Volts / 2.0f), fCosineVolts - (fPower8Volts / 2.0f));

		portENTER_CRITICAL_ISR(&mxWindTimer);
		
		float fAvgWindAngle = Ameter.raAvgWindAngle->add(fWindAngle);
		Ameter.raAvg12V->add(fPower12Volts);
		Ameter.raAvg8V->add(fPower8Volts);
		Ameter.raAvg5V->add(fPower5Volts);
		Ameter.raAvgWindSpeed->add(fWindSpeed);

		float fAvgWindSpeed = Ameter.raAvgWindSpeed->getAverage(),
				fAvg12Volts = Ameter.raAvg12V->getAverage(),
				fAvg8Volts = Ameter.raAvg8V->getAverage(),
				fAvg5Volts = Ameter.raAvg5V->getAverage();
				
		portEXIT_CRITICAL(&mxWindTimer);

		float fAvgWindAngleDegree = (fAvgWindAngle > 0 ? fAvgWindAngle : (2.0f * M_PI + fAvgWindAngle)) * 360.0f / (2.0f * M_PI);
		float fAvgWindAngleDegreePN180 = fAvgWindAngle * 360.0f / (2 * M_PI);
		
		mNet.SetApparentWindDirection(fAvgWindAngleDegreePN180);
		mNet.SetApparentWindSpeed(fAvgWindSpeed * 1.943844f);
		mNet.SetVolts(fAvg12Volts);
		
		N2kBridge.SendAppWindAngleSpeed(fAvgWindAngleDegree * (M_PI / 180.0f), fAvgWindSpeed);
		N2kBridge.SendBatteryVoltage(fAvg12Volts);
		
	    //1 mps = 2.2369362920544 mph
		float fWindSpeedMPH = fAvgWindSpeed * 2.2369362920544;
		
		char cDir[3];
		if (fAvgWindAngle < -1.75f * M_PI / 2.0f || 
			fAvgWindAngle > 1.75f * M_PI / 2.0f)
			strcpy(cDir, "S ");
		else if (fAvgWindAngle > -1.75f * M_PI / 2.0f && 
			fAvgWindAngle < -1.25f * M_PI / 2.0f)
			strcpy(cDir, "SW ");
		else if (fAvgWindAngle > -1.25f * M_PI / 2.0f && 
			fAvgWindAngle < -0.75f * M_PI / 2.0f)
			strcpy(cDir, "W ");
		else if (fAvgWindAngle > -0.75f * M_PI / 2.0f && 
			fAvgWindAngle < -0.25f * M_PI / 2.0f)
			strcpy(cDir, "NW");
		else if (fAvgWindAngle < 1.75f * M_PI / 2.0f && 
			fAvgWindAngle > 1.25f * M_PI / 2.0f)
			strcpy(cDir, "SE");
		else if (fAvgWindAngle < 1.25f * M_PI / 2.0f && 
			fAvgWindAngle > 0.75f * M_PI / 2.0f)
			strcpy(cDir, "E ");
		else if (fAvgWindAngle < 0.75f * M_PI / 2.0f && 
			fAvgWindAngle > 0.25f * M_PI / 2.0f)
			strcpy(cDir, "NE");
		else
			strcpy(cDir, "N");

		cDir[1] = '\0';

		print(LL_DEBUG, "Speed: %.2fMph %s %.2f° (%.2f°) (db 12v: %.2fv 8v: %.2fv 5v: %.2fv sine: %.2fv cosine: %.2fv angle: %.2f avg angle %.2f)\n", fWindSpeedMPH, &cDir[0], fAvgWindAngleDegree, fAvgWindAngleDegreePN180, fPower12Volts, fPower8Volts, fPower5Volts, fSineVolts, fCosineVolts, fWindAngle, fAvgWindAngle);
	
		delay(500);
	}	
}

Anemometer::Anemometer()
{
}

Anemometer::~Anemometer()
{
	print(LL_INFO, "Stopping the wind transducer monitor.\n");

	vTaskDelete(tAnemometer);	

	detachInterrupt(SpeedIO);
	xTimerStop(swWindTimer, portMAX_DELAY);
	xTimerDelete(swWindTimer, portMAX_DELAY);
	
	delete(raAvg12V);
	delete(raAvg8V);
	delete(raAvg5V);
	delete(raAvgWindSpeed);
	delete(raAvgWindAngle);
}

void Anemometer::Start()
{
	print(LL_INFO, "Starting the wind transducer monitor.\n");

	pinMode(Power12V, INPUT);
	pinMode(Power8V, INPUT);
	pinMode(Power5V, INPUT);
	pinMode(SpeedIO, INPUT);
	pinMode(SineADC, INPUT);
	pinMode(CosineADC, INPUT);

	raAvg12V = new RunningAverage(VoltagesAvgSamples);
	raAvg8V = new RunningAverage(VoltagesAvgSamples);
	raAvg5V = new RunningAverage(VoltagesAvgSamples);
	raAvgWindSpeed = new RunningAverage(WindSpeedAvgSamples);
	raAvgWindAngle = new runningAngle(runningAngle::RADIANS);
	raAvgWindAngle->setWeight(WindAngleSmoothing);

	swWindTimer = xTimerCreate("Windvane timer", 2500, pdTRUE, (void *) 0, cbSpeedTimer);
	xTimerStart(swWindTimer, portMAX_DELAY);

	analogReadResolution(12);
	attachInterrupt(SpeedIO, SpeedInt, FALLING);

	xTaskCreate(tfAnemometer, "Anemometer Worker", configMINIMAL_STACK_SIZE + 4000, NULL, tskIDLE_PRIORITY + 1, &tAnemometer);
}
