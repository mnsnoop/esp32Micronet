//If you make a thread its priority should be between tskIDLE_PRIORITY and tskIDLE_PRIORITY + 6. Any ISR need to be handled _quickly_ as we're bitbanging the radio.

#include "micronet.h" //Micronet
#include "nmea2kbridge.h" //N2k <-> Micronet bridge
#include "anemometer.h" //Hardwired raymarine wind transducer
#include "paddlewheel.h" //Speed transducer

void setup() 
{
	Serial.begin(115200);
	while (!Serial);

	delay(1000);

	PrintInitialize();
	
	SetLogLevel(LL_VERBOSE);
//	SetLogLevel(LL_INFO);

	print(LL_INFO, "Starting ESP32 Micronet.\n");

	mNet.Start();
	N2kBridge.Start();
	Ameter.Start();
	Pwheel.Start();
}

void loop() 
{
	delay(100000);
}
