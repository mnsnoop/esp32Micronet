//If you make a thread its priority should be between tskIDLE_PRIORITY and tskIDLE_PRIORITY + 6. Any ISR need to be handled _quickly_ as we're bitbanging the radio.
//Any task using analogread should be pinned to core 0 as it can interfere with bitbanging to the radio. https://esp32.com/viewtopic.php?t=7173

#include "micronet.h" //Micronet
#include "nmea2kbridge.h" //N2k <-> Micronet bridge
#include "anemometer.h" //Hardwired raymarine wind transducer
#include "paddlewheel.h" //Speed transducer

void( *resetFunc) (void) = 0;

void setup() 
{
	Serial.begin(115200);
	while (!Serial);

	delay(100);

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
	delay(10 * 60 * 1000);
	print(LL_INFO, "Auto restarting after 10 minutes.\n");
	delay(100);
	resetFunc();
}
