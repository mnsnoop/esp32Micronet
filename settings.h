#ifndef _SETTINGS_H
#define _SETTINGS_H

//#define CC1000
#define CC1101

#define NA
//#define EU

//CC1000 Pin Settings
#define PIN_PALE	16
#define PIN_PDATA	17
#define PIN_PCLOCK	5
#define PIN_DCLOCK	18
#define PIN_DDATA	19
#define PIN_OP		21

//CC1101 Pin Settings
#define PIN_MISO	19
#define PIN_MOSI	18
#define PIN_SCLK	5
#define PIN_CS		17
#define PIN_G0		16

#ifdef NA
	//CC1101 NA 915Mhz
	#define CC1101_FREQWORD2	0x23
	#define CC1101_FREQWORD1	0x3A
	#define CC1101_FREQWORD0	0x4C
#endif

#ifdef EU
	//CC1101 EU 869 Mhz
	#define CC1101_FREQWORD2	0x21
	#define CC1101_FREQWORD1	0x75
	#define CC1101_FREQWORD0	0x32
#endif

//See below for values. I found that a good CC1101 can easily overpower its output stage so you'll need to turn down the power.
//If your device is really close you might need N20.
//You can find your own sweet spot by going to setup->health->type 8 on a display and playing with the setting. You'll have to exit
//and reenter the health menu on the display each time you restart the esp32 to trigger the node info packet send.
#define CC1101_POWER		CC1101_PATABLE_P7

//Set your Network ID here. Device Id can be whatever you want.
#define MN_NETWORK_ID0	0x01 //First byte of your network ID
#define MN_NETWORK_ID1	0x00 //Second byte of your network ID
#define MN_NETWORK_ID2	0x00 //Third byte of your network ID
#define MN_NETWORK_ID3	0x00 //Fourth byte of your network ID
#define MN_DEVICE_ID0	0x01 //First byte of your device ID. Can be anything.
#define MN_DEVICE_ID1	0x00 //Second byte of your device ID. Can be anything.
#define MN_DEVICE_ID2	0x00 //Third byte of your device ID. Can be anything.
#define MN_DEVICE_ID3	0x00 //Fourth byte of your device ID. Can be anything.

#endif // _SETTINGS_H
