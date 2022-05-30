//If you make a thread its priority should be between tskIDLE_PRIORITY and tskIDLE_PRIORITY + 6. Any ISR need to be handled _quickly_ as we're bitbanging the radio.
//Any task using analogread should be pinned to core 0 as it can interfere with bitbanging to the radio. https://esp32.com/viewtopic.php?t=7173

#include <Arduino.h>
#include "BoardConfig.h"
#include "Globals.h"
#include "micronet.h" //Micronet
#ifdef HAS_N2KBRIDGE
#include "nmea2kbridge.h" //N2k <-> Micronet bridge
#endif
#ifdef HAS_ANEMOMETER
#include "anemometer.h" //Hardwired raymarine wind transducer
#endif
#ifdef HAS_PADDLEWHEEL
#include "paddlewheel.h" //Speed transducer
#endif
#ifdef HAS_GNSS
#include "NmeaDecoder.h"
#endif

//void (*resetFunc) (void) = 0;

void setup()
{
	// Init USB serial link
	USB_CONSOLE.begin(USB_BAUDRATE);

	delay(100);

	PrintInitialize();

//	SetLogLevel(LL_VERBOSE);
	SetLogLevel(LL_INFO);

	print(LL_INFO, "Starting ESP32 Micronet.\n");

	mNet.Start();
#ifdef HAS_N2KBRIDGE
	N2kBridge.Start();
#endif
#ifdef HAS_ANEMOMETER
	Ameter.Start();
#endif
#ifdef HAS_PADDLEWHEEL
	Pwheel.Start();
#endif

#ifdef HAS_GNSS
	// Init GNSS NMEA serial link
	GNSS_SERIAL.begin(GNSS_BAUDRATE, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

#ifdef HAS_COMPASS
	CONSOLE.print("Initializing navigation compass ... ");
	if (!gNavCompass.Init())
	{
		CONSOLE.println("NOT DETECTED");
	}
	else
	{
		CONSOLE.print(gNavCompass.GetDeviceName().c_str());
		CONSOLE.println(" Found");
	}
#endif
#endif
}

void loop()
{
//	delay(10 * 60 * 1000);
//	print(LL_INFO, "Auto restarting after 10 minutes.\n");
//	delay(100);
//	resetFunc();
}

void IRAM_ATTR GNSS_CALLBACK()
{
	// This callback is called each time we received data from the NMEA GNSS
	while (GNSS_SERIAL.available() > 0)
	{
		gGnssDecoder.PushChar(GNSS_SERIAL.read());
	}
}
