#ifndef _MICRONET_H
#define _MICRONET_H

#include "radio.h"

enum _eMNStatus //network states. main driver of wat do.
{
	MNS_NetworkChoice,			//Network Choice mode. How displays act. Listen to see if a network is running join if found or start one if not.
	MNS_Force_Node,				//Forced node mode. How hull transmitters act. Wake up to listen for a sync packet once a second. If none is detected go back to sleep. If we detect a network join the network.
	MNS_Force_Scheduler,		//Forced schedule mode. *fixme not implimented* scan for a network and if one is detected take over scheduler duties next 0x0a/0x0b exchange.
//above are the start values you can pick.
	MNS_NetworkChoice_S1,		//Listening for 1 second.
	MNS_NetworkChoice_S2,		//Listening for another second.
	MNS_NetworkChoice_M1,		//If no network is found announce ourselves as the scheduler to wake up other nodes and start sending sync packets.
	MNS_NetworkChoice_M2,		//Wait for our announcement to finish.
	MNS_NetworkChoice_N1,		//Network found. send out 0x03 packets till we get on the list. *fixme not implimented*
	MNS_Force_Node_Listening,	//Forced node mode listening for a network
	MNS_Connected,				//This is a marker to seperate unconnected and connected states for easy status checks. Not a valid state.
	MNS_NetworkChoice_Node,		//Network Choice node mode. *fixme not implimented*
	MNS_NetworkChoice_Scheduler,//Network Choice scheduler mode. *fixme not implimented*
	MNS_Force_Node_Connected,	//Forced node connected to a network. *fixme not implimented*
	MNS_Force_Scheduler_Connected //*fixme not implimented*
};

class _cMNId
{
	public:
	byte bId[4];

	bool operator ==(const _cMNId &id2)
	{
		if (bId[0] == id2.bId[0] && bId[1] == id2.bId[1] && bId[2] == id2.bId[2] && bId[3] == id2.bId[3])
			return true;
		else
			return false;
	}
	
	bool operator !=(const _cMNId &id2)
	{
		if (bId[0] != id2.bId[0] || bId[1] != id2.bId[1] || bId[2] != id2.bId[2] || bId[3] != id2.bId[3])
			return true;
		else
			return false;
	}
};

union _uMNPacket
{
	struct __attribute__((__packed__))
	{
		_cMNId mnIdNetwork;
		_cMNId mnIdDevice;
		byte bPacketType;
		byte bUnk1;
		byte bSignalQuality;
		byte bChecksum;
		byte bLength;
		byte bLengthChecksum;
		byte bData[MN_MAXPACKETSIZE - 14];
	} sHeader;
	byte bRaw[MN_MAXPACKETSIZE];
};

struct _sMNPSync
{
	_cMNId mnIdDevice;
	byte bWindowSize;
};

struct _sDataArrayIn //Micronet -> (...)
{
	uint16_t iSpeed;
	uint64_t tlSpeedUpdate = 0;
	
	int8_t iWaterTemp;
	uint64_t tlWaterTempUpdate = 0;
	
	uint16_t iDepth;
	uint64_t tlDepthUpdate = 0;
	
	uint16_t iAppWindSpeed;
	uint64_t tlAppWindSpeedUpdate = 0;
	
	int16_t iAppWindDir;
	uint64_t tlAppWindDirUpdate = 0;
	
	int16_t iHeading;
	uint64_t tlHeadingUpdate = 0;
		
	uint16_t iVolts;
	uint64_t tlVoltsUpdate = 0;
};

struct _sDataArrayOut //(...) -> Micronet
{
	uint16_t iSpeed;
	uint64_t tlSpeedUpdate = 0;
	
	uint32_t iTrip;
	uint32_t iLog;
	uint64_t tlTripLogUpdate = 0;
	
	int8_t iWaterTemp;
	uint64_t tlWaterTempUpdate = 0;
	
	uint16_t iDepth;
	uint64_t tlDepthUpdate = 0;
	
	uint16_t iAppWindSpeed;
	uint64_t tlAppWindSpeedUpdate = 0;
	
	int16_t iAppWindDir;
	uint64_t tlAppWindDirUpdate = 0;
	
	int16_t iHeading;
	uint64_t tlHeadingUpdate = 0;
	
	uint16_t iSOG; 
	uint16_t iCOG;
	uint64_t tlSOGCOGUpdate = 0;
	
	uint8_t iLatDegree;
	uint16_t iLatMinute;
	uint8_t iLongDegree;
	uint16_t iLongMinute;
	uint8_t iNESW;
	uint64_t tlLatLongUpdate = 0;
	
	uint16_t iBTW;
	char	cWPName[16];
	short	iWPNameScrollPos;
	uint64_t tlBTWUpdate = 0;

	int16_t iXTE;
	uint64_t tlXTEUpdate = 0;

	uint8_t iHour;
	uint8_t iMinute;
	uint64_t tlTimeUpdate = 0;
	
	uint8_t iDay;
	uint8_t iMonth;
	uint8_t iYear;
	uint64_t tlDateUpdate = 0;
	
	uint16_t iVMG_WP;
	uint64_t tlVMG_WPUpdate = 0;
	
	uint16_t iVolts;
	uint64_t tlVoltsUpdate = 0;
	
	uint32_t iDTW;
	uint64_t tlDTWUpdate = 0;

	uint8_t bNodeInfoType;
	uint8_t bNodeInfoVMajor;
	uint8_t bNodeInfoVMinor;
	bool bSendNodeInfo;
};

struct _sMNetHandler
{
	void (*cbCallback)(_sDataArrayIn sDataIn, void *v2);
	void *vArg;
};

class Micronet
{
	private:
	TaskHandle_t thMicronetWorker,
				 thIncomingPacketHandler;
				
	QueueHandle_t *qhIncomingPacket,
				  *qhOutgoingPacket;

	TimerHandle_t tiMicronet;

	_sMNetHandler sIncomingPacketHandlers[10];
	_sMNetHandler sTimerHandlers[10];

	_eMNStatus eMNStatus;
	
	uint64_t tLAnnounce;
	int iSyncPacketOffset; //offset from tickcount for when we expect sync packets (in us).

	_sMNPSync sMNSync[MN_MAXNODES];
	int iMNNodes;
	uint64_t tNextPing;
	uint64_t tCommandBlock;
	
	_sDataArrayIn sDataArrayIn;
	_sDataArrayOut sDataArrayOut;
	
	void IncomingPacketHandler(_sPacket *p); //handling of incoming packets.
	void MicronetWorker(); //runs during network deadtime.
	
	static void IncomingPacketHandlerCB(void *m);
	static void MicronetWorkerThread(void *m);
	static void SentPacketCallback(void *m, _sPacket *p, bool bSent);

	friend void MicronetTimerCallback(TimerHandle_t xTimer);

	int GetPacketWindow(_cMNId mnDevice, byte bPacketType, bool bFuture = false); //returns the window offset in us for the current or next round.
	uint64_t GetNextPacketWindow(_cMNId mnDevice, byte bPacketType) //returns the window offset in us for the next round.
	{
		return GetPacketWindow(mnDevice, bPacketType, true) + tLAnnounce;
	}

	int FindMNDeviceInSyncList(_cMNId mnDevice);
	
	void CreatePacket0x01(_uMNPacket *pPacket);
	void CreatePacket0x02(_uMNPacket *pPacket);
	void CreatePacket0x03(_uMNPacket *pPacket);
	void CreatePacket0x05(_uMNPacket *pPacket, int iNewWindowSize);
	void CreatePacket0x07(_uMNPacket *pPacket);
	void CreatePacket0x0B(_uMNPacket *pPacket);

	public:
	Radio CCRadio;
	
	Micronet();
	~Micronet();

	void Start();
	
	void AddTimerHandler(void (*cbCallback)(_sDataArrayIn sDataIn, void *v2), void *vArg); //access to timer. this fires every second during network dead time.
	void RemoveTimerHandler(void (*cbCallback)(_sDataArrayIn sDataIn, void *v2));

	//tSendTime is when the end of the syncword is sent, not the start of the preamble.
	void Send(_uMNPacket pPacket, uint64_t tSendTime, int iPreambleLength = MN_PREAMBLELEN);

	TaskHandle_t GetWorkerTaskHandle()
	{
		return thMicronetWorker;
	}

	TaskHandle_t GetIncomingPacketTaskHandle()
	{
		return thIncomingPacketHandler;
	}
	
	byte GetCRC(byte *b, int len);
	void PrintPacket(_sPacket *p, bool bSR, int iPrediction);
	void PrintPacket(_sPacket *p, bool bSR)
	{
		PrintPacket(p, bSR, 0);
	}

	void SetSpeed(float fSpeed);
	void SetTripLog(float fTrip, float fLog);
	void SetWaterTemperature(float fTemp);
	void SetDepth(float fDepth);
	void SetApparentWindSpeed(float fWindSpeed);
	void SetApparentWindDirection(float fWindDir);
	void SetHeading(float fHeading);
	void SetSOGCOG(float fSOG, float fCOG);
	void SetLatLong(float fLat, float fLong);
	void SetBTW(float fBTW);
	void SetWPName(char *cWPName);
	void SetXTE(float fXTE);
	void SetTime(uint8_t iHour, uint8_t iMinute);
	void SetDate(uint8_t iDay, uint8_t iMonth, uint8_t iYear);
	void SetVMG_WP(float fSpeed);
	void SetVolts(float fVolts);
	void SetDTW(float fDistance);
	
	//fixme: put these behind functions.
	_cMNId	mnIdMyNetwork,
		 	mnIdMyDevice;
	
	bool bPrintSends = true,
		 bPrintReceives = true;
};

extern Micronet mNet;

float readFloat(unsigned int addr);
void writeFloat(unsigned int addr, float x);

#endif // _MICRONET_H


/* 
 *  These notes are chaotic. 
 *  Some info maybe old or wrong.
 * 
 * Abandon all hope.

Nodes remember the sync list and on start will use thier memorized list instead of the one we send...

Basic time specs.
 13.02 us per bit
104.17 us per byte
  1771 us for 17 bytes (standard preamble)

Max packet size: 200
Max nodes: 32

NW = Network Id
DI = Device Id
UK = Unknown, believed to be a reciever mask or sender status bits.
NQ = Network quality (RSSI) for responce packets. 0-9 scale, 0 being didn't recieve the packet, 9 being strongest RSSI.
CS = Checksum. Previous bytes added togeather from the last checksum limited to one byte.
LN = Total packet length minus the LN fields.

0x01 Network synchronization packet. 
-First ring.
NW NW NW NW DI DI DI DI *01* UK NQ CS LN CS *81 00 01 00 00 00 00 12*

0x02 Data packet.
-Second ring.
NW NW NW NW DI DI DI DI *02* UK NQ CS LN CS *04 04 05 13 89 A9 04 1B 05 00 78 9C*

0x03 Add transmit slot packet.
-Third ring.
-Can and does collide with other third ring packets.
NW NW NW NW DI DI DI DI *03* UK NQ CS LN CS *00 1A 1A*

0x04 Remove transmit slot packet.
-Second ring.
NW NW NW NW DI DI DI DI *04* UK NQ CS LN CS 

0x05 Adjust transmit slot packet.
-Second ring.
NW NW NW NW DI DI DI DI *05* UK NQ CS LN CS *1A 1A*

0x06 Command packet.
-Third ring.
-Every node should reply with an 0x07. If not the node keeps sending the 0x06 for 5 times.
-Only one command packet can be handled at a second.
NW NW NW NW DI DI DI DI *06* UK NQ CS LN CS *E5 00 10 F5*

0x07 Announcement/Ack.
-Forth ring.
-Forth ring runs backwards.
NW NW NW NW DI DI DI DI *07* UK NQ CS LN CS 

0x08 Node leaving network.
-Second ring? (verify me pls)
NW NW NW NW DI DI DI DI *08* UK NQ CS LN CS 

0x09 Unknown, unseen, no reply.
NW NW NW NW DI DI DI DI *09* UK NQ CS LN CS 

0x0A Network status ping.
-Third ring, network controller only
-Used to cull dead nodes from sync list.
-Also checks to see if there is a device that wants to take over as scheduler.
NW NW NW NW DI DI DI DI *0A* UK NQ 7E LN CS 

0x0B Netowrk status pong. 3rd ring
-Forth ring.
-Forth ring runs backwards.
NW NW NW NW DI DI DI DI *0B* UK NQ 01 LN CS *00 00*

0x18 Passing scheduling duty to another device. The device's id reversed is the data.
NW NW NW NW DI DI DI DI *18* UK NQ CS LN CS *7C 62 03 81 62*

0x19 Acknolegement of passing scheduling duty by device taking over.
NW NW NW NW DI DI DI DI *19* UK NQ CS LN CS 


0x01 Network synchronization packet.
-First device in the list is always the scheduler and the it's window size is 3 + (5 * iMNNodes).
-The network sync packet has a hard limit of ~32ms length. 

0x02 Data packets.
-Types:
-0x01 - 06 byte Speed                0x04, 0x01, 0x05, 0x00, 0x64, 0x6E
-0x02 - 12 byte Trip/Log             0x0A, 0x02, 0x05, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x0A, 0x7F
-0x03 - 05 byte Temp                 0x03, 0x03, 0x05, 0x02, 0x0D
-0x04 - 06 byte Depth                0x04, 0x04, 0x05, 0x00, 0x0A, 0x17
-0x05 - 06 byte App Wind Speed       0x04, 0x05, 0x05, 0x00, 0x0A, 0x18
-0x06 - 06 byte App Wind Dir         0x04, 0x06, 0x05, 0x00, 0x01, 0x10
-0x07 - 06 byte HDG                  0x04, 0x07, 0x05, 0x00, 0x01, 0x11
-0x08 - 08 byte SOG/COG              0x06, 0x08, 0x05, 0x00, 0x0A, 0x00, 0x01, 0x1E
-0x09 - 11 byte LAT/LONG             0x09, 0x09, 0x05, 0x01, 0x03, 0xE8, 0x01, 0x03, 0xE8, 0x00, 0xEF
-0x0A - 12 byte BTW                  0x0A, 0x0A, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x21
-0x0B - 06 byte XTE                  0x04, 0x0B, 0x05, 0x00, 0x64, 0x78
-0x0C - 06 byte Time                 0x04, 0x0C, 0x05, 0x01, 0x00, 0x16
-0x0D - 07 byte Date                 0x05, 0x0D, 0x05, 0x01, 0x01, 0x01, 0x1A
- ...
-0x10 - 08 byte Node Info            0x06, 0x10  0x01, 0x02, 0x20, 0x23, 0x08, 0x64
- ...
-0x12 - 06 byte VMG-WP               0x04, 0x12, 0x05, 0x00, 0x64, 0x7F 
- ...
-0x1B - 06 byte Battery              0x04, 0x1B, 0x05, 0x00, 0x0A, 0x2E
- ...
-0x1F - 08 byte DTW                  0x06, 0x1F, 0x05, 0x00, 0x00, 0x00, 0x01, 0x2B
- ...
-0xFD - 08 byte timer stop/start/set	0x06, 0xFD, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04

-TWDIR needs App Wind Speed, App Wind Dir, HDG
-BEAUF needs App Wind Speed, App Wind Dir, HDG
-MAG TWDIR needs App Wind Speed, App Wind Dir, HDG
-HEAD/LIFT needs App Wind Speed, App Wind Dir, HDG (this one takes a little bit to fill in as its a running average)
-VMG needs App Wind Speed, App Wind Dir, Speed
-TRUE WIND SPEED needs App Wind Speed, App Wind Dir, Speed
-TRUE WIND DIR needs App Wind Speed, App Wind Dir, Speed
-ETA/TTG needs VMG-WP and DTW.

0x03 Add transmit slot packet.
-Data is an unknown 0x00 byte followed by the window size you want then a checksum. Repeating this packet with a different window size does not change your window size.
-First byte might be an 0x0A byte to signal wanting sync duty. Need to check.

0x04 Remove transmit slot packet.
-Send this will remove your node's id from the sync list.

0x05 Adjust transmit slot packet.
-Data is one byte which is the new window size and a checksum.

0x06 Command packets.

0xE5 0x00 Display backlight command.
Backlight Off: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 00 10 F5 
Backlight  L1: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 01 0D F3 
Backlight  L2: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 02 0E F5 
Backlight  L3: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 03 0F F7 

0xFF Settings command.
FF 01 01 00 24 25
CW RG SS SS CT CS
CW = Command word.
RG = Registry.
SS = Settings.
CT = Counter. Every time a setting is changed this increments. Probably for synchronization. Doesn't seem to matter as I've sent out 00 multiple times and the display took the setting.
CS = Checksum.

0xFF Reg 0x00 Speed adj. +/- 200%
    speed +1%: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 33 6D A0 
	speed +2%: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 34 6E A2 
  speed +200%: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 FA 6F 69 

0xFF Reg 0x01 General Settings.
Default settings (all zeros) are: Depth feet, Speed knots, Wind knots, Log units nm. Is bit based. Not fully mapped.
      Default: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 00 24 25 0000 0001 0000 00001 0000 0000
  Speed   mph: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 01 30 32 0000 0001 0000 00001 0000 0001
  Speed  kmph: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 02 2F 32 0000 0001 0000 00001 0000 0010
 Log units sm: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 04 26 2B 0000 0001 0000 00001 0000 0100
 Log units  m: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 08 25 2E 0000 0001 0000 00001 0000 1000 
Depth  meters: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 10 13 24 0000 0001 0000 00001 0001 0000
Depth fathoms: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 20 15 36 0000 0001 0000 00001 0010 0000
  Wind meters: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 40 1F 60 0000 0001 0000 00001 0100 0000

0xFF Reg 0x02 Sea temp adj. +/- 10c
     temp .5c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 01 6B 70
     temp +1c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 02 6C 70 
	temp +10c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 14 6E 84 

0xFF Reg 0x0B More General Settings/Some alarm settings. Bit based. Not fully mapped.
       Temp f: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 01 2D 39 0000 1011 0000 00001 0000 0001 (default is c)
 WP Arv Alarm: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 10 5B 76 0000 1011 0000 00001 0001 0000
    Alarm XTE: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 20 67 92 0000 1011 0000 00001 0010 0000
Spd frmt 0.01: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 40 38 83 0000 1011 0000 00001 0100 0000 (default is 0.1)

0xFF Reg 0x11 Depth alarm deep.
    depth alarm deep 1ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 11 01 01 57 69 
    depth alarm deep 2ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 11 01 02 5A 6D 

0xFF Reg 0x12 Depth alarm shallow.
Depth alarm shallow .5ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 12 01 05 54 6B 
Depth alarm shallow  1ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 12 01 0A 59 75 

0xFF Reg 0x13 CRSE Alarm.
CRSE Alarm 1 degree : 01 0A 8B 60 81 03 61 7C 06 09 00 66 13 13 FF 13 02 01 00 5D 72
CRSE Alarm 2 degrees: 01 0A 8B 60 81 03 61 7C 06 09 00 66 13 13 FF 13 02 02 00 5E 74 


0x07 Announcement/Ack

All commands should get a 07 reply from all nodes


0x08 Node leaving network

During pairing when a node is leaving a network for a new one it sends 0x08 packets to the old network.


0x09 Unknown unseen no reply


0x0A Network status ping.

Part of device culling and master switching. The scheduler will send these out until it gets a responce from all devices in the list.


0x0B Network status pong.

Rsponce is the level of importance of a device. If a device with a higher number then the one running the network replies then sync duties gets passed on to it.
00 - how a hull transmitter responses
10 - how a slave display reponds
90 - how a master display responds


0x18 Passing scheduling duty to another device. 

The device's id reversed is the data. Device needs to respond with a 0x19 packet.


0x19 Acknolegement of passing scheduling duty by device taking over.

The current scheduler will send out 4 more sync packets before the switchover happens.



Notes:

Pairing

Status bit change, 5+ second preamble, 60 seconds join peroid with special status bit (0f wait 0d join). Devices joining the new network that are running an old network send out 0x08 packets. After 60 seconds nodes update thier NID to the new network.
Device Id's are not static. If a device comes online to find it's id in use it'll increment its id and request a new window. 


Scheduling
first slot always the node running the network. Not seen it used. Might just be a spacer.

Micronet devices lock avgfilter after ~20 bits of preamble.


Full pairing
21:43:22.312 -> 0125.00720000: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:23.306 -> 0126.00720004: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:24.300 -> 0127.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:25.327 -> 0128.00720003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:26.321 -> 0129.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:27.315 -> 0130.00720001: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:28.309 -> 0131.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:29.303 -> 0132.00720004: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:29.336 -> 0132.00754003: Received 014 bytes. 36566μs:| 01 01 01 02 | 81 03 61 91 | 0A | 0F 00 94 | 0C 0C | 
21:43:30.330 -> 0133.00720003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:31.324 -> 0134.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:32.318 -> 0135.00720001: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 03 00 00 A8 
21:43:33.312 -> 0136.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 02 00 00 00 0A 02 00 01 48 
21:43:34.306 -> 0137.00720004: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:35.300 -> 0138.00720002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 0B 01 00 00 E8 
21:43:36.328 -> 0139.00720004: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:37.322 -> 0140.00719999: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:37.355 -> 0140.00754322: Received 018 bytes. 36620μs:| 01 01 01 02 | 81 03 61 91 | 06 | 0F 00 90 | 10 10 | E5 00 05 EA 
21:43:37.388 -> 0140.00782318: Received 014 bytes. 65083μs:| 01 01 01 02 | 81 03 61 91 | 07 | 0F 00 91 | 0C 0C | 
21:43:38.316 -> 0141.00720004: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:38.349 -> 0141.00754318: Received 018 bytes. 36518μs:| 01 01 01 02 | 81 03 61 91 | 06 | 0F 00 90 | 10 10 | E5 00 05 EA 
21:43:38.383 -> 0141.00782319: Received 014 bytes. 65084μs:| 01 01 01 02 | 81 03 61 91 | 07 | 0F 00 91 | 0C 0C | 
21:43:39.311 -> 0142.00720003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 01 00 00 A4 
21:43:39.344 -> 0142.00754320: Received 018 bytes. 36697μs:| 01 01 01 02 | 81 03 61 91 | 06 | 0F 00 90 | 10 10 | E5 00 05 EA 
21:43:39.377 -> 0142.00782322: Received 014 bytes. 65186μs:| 01 01 01 02 | 81 03 61 91 | 07 | 0F 00 91 | 0C 0C | 
21:43:40.305 -> 0143.00720322: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 00 05 02 00 01 48 
21:43:40.338 -> 0143.00754321: Received 018 bytes. 36699μs:| 01 01 01 02 | 81 03 61 91 | 06 | 0F 00 90 | 10 10 | E5 00 05 EA 
21:43:40.371 -> 0143.00782322: Received 014 bytes. 65292μs:| 01 01 01 02 | 81 03 61 91 | 07 | 0F 00 91 | 0C 0C | 
21:43:41.332 -> 0144.00720321: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0F 00 8B | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 08 02 00 00 00 0A 02 00 01 48 
21:43:41.365 -> 0144.00754320: Received 018 bytes. 36592μs:| 01 01 01 02 | 81 03 61 91 | 06 | 0F 00 90 | 10 10 | E5 00 05 EA 
21:43:42.326 -> 0145.00720001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:43.320 -> 0146.00720003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:44.314 -> 0147.00720004: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:45.308 -> 0148.00720003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 BB 
21:43:46.302 -> 0149.00720004: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:47.329 -> 0150.00720000: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:48.324 -> 0151.00720002: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 07 02 00 00 00 08 02 00 01 32 
21:43:49.318 -> 0152.00720002: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:50.312 -> 0153.00720003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:50.345 -> 0153.00749001: Received 014 bytes. 30926μs:| 01 0A 8B 60 | 01 0B 8B 61 | 08 | 01 00 F7 | 0C 0C | 
21:43:51.306 -> 0154.00720003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:52.301 -> 0155.00719999: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:53.328 -> 0156.00720003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 01 9B 
21:43:54.322 -> 0157.00720001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:54.355 -> 0157.00749002: Received 014 bytes. 30868μs:| 01 0A 8B 60 | 01 0B 8B 61 | 08 | 01 00 F7 | 0C 0C | 
21:43:55.316 -> 0158.00720321: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:56.310 -> 0159.00720319: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 06 02 00 00 00 08 02 00 01 32 
21:43:57.303 -> 0160.00720321: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:58.298 -> 0161.00720316: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:43:59.325 -> 0162.00720324: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 BA 
21:43:59.358 -> 0162.00749001: Received 014 bytes. 30857μs:| 01 0A 8B 60 | 01 0B 8B 61 | 08 | 01 00 F7 | 0C 0C | 
21:44:00.319 -> 0163.00720320: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:01.313 -> 0164.00720320: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:02.308 -> 0165.00720322: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 02 00 01 32 
21:44:02.341 -> 0165.00748999: Received 014 bytes. 30827μs:| 01 0A 8B 60 | 01 0B 8B 61 | 08 | 01 00 F7 | 0C 0C | 
21:44:03.301 -> 0166.00720321: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:04.329 -> 0167.00721001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:05.323 -> 0168.00721003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 08 02 00 01 BB 
21:44:06.317 -> 0169.00721001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:07.311 -> 0170.00720999: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:08.305 -> 0171.00721001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:08.338 -> 0171.00749003: Received 014 bytes. 30791μs:| 01 0A 8B 60 | 01 0B 8B 61 | 08 | 01 00 F7 | 0C 0C | 
21:44:09.332 -> 0172.00721001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:10.326 -> 0173.00721003: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 00 99 
21:44:11.320 -> 0174.00721002: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 00 01 B2 
21:44:11.354 -> 0174.00750003: Received 017 bytes. 31034μs:| 01 01 01 02 | 01 0B 8B 61 | 03 | 01 09 0A | 0F 0F | 01 1A 1B 
21:44:12.315 -> 0175.00721317: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:12.348 -> 0175.00743998: Received 026 bytes. 24332μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:13.309 -> 0176.00721320: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:13.342 -> 0176.00744002: Received 026 bytes. 24440μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:14.303 -> 0177.00721321: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:14.336 -> 0177.00744002: Received 026 bytes. 24461μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:15.331 -> 0178.00721317: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:15.331 -> 0178.00744003: Received 026 bytes. 24203μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:16.325 -> 0179.00721318: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:16.325 -> 0179.00744320: Received 026 bytes. 24318μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:17.319 -> 0180.00721999: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:17.352 -> 0180.00744321: Received 026 bytes. 24395μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:18.314 -> 0181.00722001: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:18.347 -> 0181.00744002: Received 026 bytes. 24182μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:19.308 -> 0182.00722001: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:19.341 -> 0182.00744319: Received 026 bytes. 24346μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:20.302 -> 0183.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 02 1B 8B 61 1A 00 00 B0 
21:44:20.335 -> 0183.00744319: Received 026 bytes. 24417μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:21.329 -> 0184.00722001: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:21.329 -> 0184.00744317: Received 026 bytes. 24398μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:22.324 -> 0185.00721999: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 08 02 02 17 16 C2 34 00 01 60 
21:44:22.324 -> 0185.00744320: Received 026 bytes. 24373μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:23.318 -> 0186.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 03 0B 8E C2 34 00 01 60 
21:44:23.351 -> 0186.00744317: Received 026 bytes. 24368μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:24.313 -> 0187.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:24.346 -> 0187.00744318: Received 026 bytes. 24374μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:25.306 -> 0188.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:25.340 -> 0188.00744317: Received 026 bytes. 24428μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:26.301 -> 0189.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:26.334 -> 0189.00745001: Received 026 bytes. 24474μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:27.329 -> 0190.00722000: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:27.329 -> 0190.00744998: Received 026 bytes. 24380μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:28.323 -> 0191.00722003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 03 0B 8E C2 34 00 01 60 
21:44:28.323 -> 0191.00745001: Received 026 bytes. 24485μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:29.317 -> 0192.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:29.350 -> 0192.00745001: Received 026 bytes. 24321μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:30.311 -> 0193.00722003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:30.344 -> 0193.00745002: Received 026 bytes. 24199μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:31.305 -> 0194.00722002: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:31.338 -> 0194.00745001: Received 026 bytes. 24306μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:32.299 -> 0195.00721999: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:32.332 -> 0195.00744998: Received 026 bytes. 24218μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:33.327 -> 0196.00722003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:33.327 -> 0196.00745002: Received 026 bytes. 24305μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:34.321 -> 0197.00722003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:34.321 -> 0197.00745001: Received 026 bytes. 24335μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:35.315 -> 0198.00722003: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:35.348 -> 0198.00745002: Received 026 bytes. 24329μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:36.309 -> 0199.00722321: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:36.342 -> 0199.00745003: Received 026 bytes. 24282μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:37.303 -> 0200.00722318: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:37.336 -> 0200.00744998: Received 026 bytes. 24308μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:38.330 -> 0201.00722316: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:38.330 -> 0201.00745002: Received 026 bytes. 24305μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:39.325 -> 0202.00722319: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:39.325 -> 0202.00745001: Received 026 bytes. 24336μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:40.319 -> 0203.00722316: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:40.352 -> 0203.00745002: Received 026 bytes. 24480μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:41.313 -> 0204.00722318: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 0D 00 89 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:41.346 -> 0204.00745001: Received 026 bytes. 24356μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:42.307 -> 0205.00722318: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:42.340 -> 0205.00744997: Received 026 bytes. 24309μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:42.340 -> 0205.00760998: Received 032 bytes. 39308μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 00 0D 32 00 00 DD 00 00 00 00 00 00 00 00 00 06 21 
21:44:42.373 -> 0205.00765997: Received 014 bytes. 46490μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:42.373 -> 0205.00787319: Received 014 bytes. 67875μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:43.301 -> 0206.00722317: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:43.334 -> 0206.00745318: Received 026 bytes. 24457μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:43.367 -> 0206.00761002: Received 032 bytes. 39338μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 00 0D 32 00 00 DD 00 00 00 00 00 00 00 00 00 06 21 
21:44:43.367 -> 0206.00766316: Received 014 bytes. 46720μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:43.367 -> 0206.00787319: Received 014 bytes. 67983μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:44.328 -> 0207.00722318: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:44.328 -> 0207.00745002: Received 026 bytes. 24307μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:44.361 -> 0207.00761001: Received 032 bytes. 39311μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 00 0D 32 00 00 DD 00 00 00 00 00 00 00 00 00 06 21 
21:44:44.361 -> 0207.00766001: Received 014 bytes. 46416μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:44.394 -> 0207.00787319: Received 014 bytes. 67879μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:45.322 -> 0208.00723000: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 04 01 01 0B 8B 61 1A 00 00 B0 
21:44:45.322 -> 0208.00745002: Received 026 bytes. 24263μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:45.355 -> 0208.00761001: Received 032 bytes. 39310μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 00 0D 32 00 00 DD 00 00 00 00 00 00 00 00 00 06 62 
21:44:45.355 -> 0208.00766001: Received 014 bytes. 46389μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:45.388 -> 0208.00787318: Received 014 bytes. 67852μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:46.316 -> 0209.00723000: Received 042 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 28 28 | 81 03 61 91 1C 00 00 00 02 01 00 00 00 03 01 00 00 00 0C 01 01 0B 8B 61 1A 00 00 B0 
21:44:46.350 -> 0209.00745314: Received 026 bytes. 24537μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:46.350 -> 0209.00761002: Received 032 bytes. 39311μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 00 0D 32 00 00 DD 00 00 00 00 00 00 00 00 00 0E 21 
21:44:46.383 -> 0209.00766316: Received 014 bytes. 46695μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:47.310 -> 0210.00721997: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 01 0B 8B 61 1A 00 00 A6 
21:44:47.344 -> 0210.00739314: Received 026 bytes. 18564μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:47.344 -> 0210.00754999: Received 032 bytes. 33526μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 0D 0D 00 0A 05 00 00 00 00 00 00 00 00 00 00 07 2F 
21:44:47.377 -> 0210.00760320: Received 014 bytes. 40827μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:47.377 -> 0210.00776318: Received 014 bytes. 56670μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:48.304 -> 0211.00722000: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 01 0B 8B 61 1A 00 00 A6 
21:44:48.337 -> 0211.00739315: Received 026 bytes. 18404μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:48.337 -> 0211.00755001: Received 032 bytes. 33502μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 0D 0D 00 0A 05 00 00 00 00 00 00 00 00 00 00 07 2F 
21:44:48.371 -> 0211.00760315: Received 014 bytes. 40563μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:48.371 -> 0211.00776317: Received 014 bytes. 56647μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:49.298 -> 0212.00722001: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 01 0B 8B 61 1A 00 00 A6 
21:44:49.332 -> 0212.00740001: Received 026 bytes. 18604μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:49.332 -> 0212.00755000: Received 032 bytes. 33452μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 0D 0D 00 0A 05 00 00 00 00 00 00 00 00 00 00 07 2F 
21:44:49.365 -> 0212.00760329: Received 014 bytes. 40814μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:49.365 -> 0212.00776320: Received 014 bytes. 56647μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:50.326 -> 0213.00722002: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 01 0B 07 61 1A 00 00 8C 
21:44:50.326 -> 0213.00740000: Received 026 bytes. 18538μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:50.359 -> 0213.00755001: Received 032 bytes. 33449μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 0D 0D 00 0A 05 00 00 00 00 00 00 00 00 00 00 07 2E 
21:44:50.359 -> 0213.00761000: Received 014 bytes. 40823μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:50.359 -> 0213.00776318: Received 014 bytes. 56644μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:51.320 -> 0214.00722318: Received 037 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 23 23 | 81 03 61 91 17 00 00 00 02 01 00 00 00 03 01 01 0B 8B 61 1A 00 00 A6 
21:44:51.320 -> 0214.00740001: Received 026 bytes. 18603μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:51.353 -> 0214.00755000: Received 032 bytes. 33450μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1E 1E | FF 0D 0D 00 0A 05 00 00 00 00 00 00 00 00 00 00 07 2F 
21:44:51.353 -> 0214.00761001: Received 014 bytes. 40888μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:52.314 -> 0215.00721997: Received 032 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 1E 1E | 81 03 61 91 12 00 00 00 02 01 01 0B 8B 61 1A 00 00 9D 
21:44:52.314 -> 0215.00733999: Received 026 bytes. 12670μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:52.347 -> 0215.00748998: Received 029 bytes. 27671μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1B 1B | FF 1A 0A 00 5A B4 00 00 00 00 00 00 00 08 39 
21:44:52.347 -> 0215.00754998: Received 014 bytes. 34875μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:52.347 -> 0215.00765319: Received 014 bytes. 45441μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:53.308 -> 0216.00722000: Received 032 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 1E 1E | 81 03 61 91 12 00 00 00 02 01 01 0B 8B 61 1A 00 00 9D 
21:44:53.341 -> 0216.00734002: Received 026 bytes. 12741μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:53.341 -> 0216.00749001: Received 029 bytes. 27669μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1B 1B | FF 1A 0A 00 5A B4 00 00 00 00 00 00 00 08 7B 
21:44:53.341 -> 0216.00755002: Received 014 bytes. 35004μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:53.374 -> 0216.00765316: Received 014 bytes. 45494μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:54.303 -> 0217.00722001: Received 032 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 1E 1E | 81 03 61 91 12 00 00 00 02 01 01 1F 16 C2 34 00 01 3A 
21:44:54.336 -> 0217.00734001: Received 026 bytes. 12613μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:54.336 -> 0217.00749000: Received 029 bytes. 27590μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1B 1B | FF 1A 0A 00 5A B4 00 00 00 00 00 00 00 08 39 
21:44:54.369 -> 0217.00755001: Received 014 bytes. 34927μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:54.369 -> 0217.00765317: Received 014 bytes. 45363μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:55.297 -> 0218.00722000: Received 032 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 1E 1E | 81 03 61 91 12 00 00 00 02 01 01 0B 8B 61 1A 00 00 9D 
21:44:55.330 -> 0218.00734001: Received 026 bytes. 12594μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:55.330 -> 0218.00749000: Received 029 bytes. 27538μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1B 1B | FF 1A 0A 00 5A B4 00 00 00 00 00 00 00 08 39 
21:44:55.363 -> 0218.00755001: Received 014 bytes. 34885μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:55.363 -> 0218.00765316: Received 014 bytes. 45338μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:56.324 -> 0219.00722000: Received 032 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 1E 1E | 81 03 61 91 12 00 00 00 02 01 01 0B 8B 61 1A 00 00 9D 
21:44:56.324 -> 0219.00734001: Received 026 bytes. 12692μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:56.324 -> 0219.00749001: Received 029 bytes. 27593μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 1B 1B | FF 1A 0A 00 5A B4 00 00 00 00 00 00 00 08 3B 
21:44:56.357 -> 0219.00755001: Received 014 bytes. 34905μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:57.318 -> 0220.00721318: Received 027 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 19 19 | 81 03 61 91 0D 01 0B 8B 61 1A 00 00 95 
21:44:57.318 -> 0220.00727997: Received 026 bytes. 06835μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:57.352 -> 0220.00741998: Received 018 bytes. 21731μs:| 01 01 01 02 | 81 03 61 91 | 06 | 09 00 8A | 10 10 | FE 24 09 2B 
21:44:57.352 -> 0220.00748997: Received 014 bytes. 29139μs:| 01 01 01 02 | 01 0B 8B 61 | 07 | 01 09 0E | 0C 0C | 
21:44:57.352 -> 0220.00754319: Received 014 bytes. 34264μs:| 01 01 01 02 | 81 03 61 91 | 07 | 09 00 8B | 0C 0C | 
21:44:58.313 -> 0221.00721318: Received 027 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 19 19 | 81 03 61 91 0D 01 0B 8B 61 1A 00 00 95 
21:44:58.313 -> 0221.00728001: Received 026 bytes. 06709μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:44:59.307 -> 0222.00721316: Received 027 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 19 19 | 81 03 61 91 0D 01 0B 8B 61 1A 00 00 95 
21:44:59.340 -> 0222.00728000: Received 026 bytes. 06805μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 
21:45:00.301 -> 0223.00721318: Received 027 bytes. 00000μs:| 01 01 01 02 | 81 03 61 91 | 01 | 09 00 85 | 19 19 | 81 03 61 91 0D 01 0B 8B 61 1A 00 00 95 
21:45:00.334 -> 0223.00728001: Received 026 bytes. 06753μs:| 01 01 01 02 | 01 0B 8B 61 | 02 | 01 09 09 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 


Hull battery alarm with no slot.

50568.054778:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50568.060469: Received 016 bytes. +006808 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50568.074469: Received 017 bytes. +020234 μs:| 01 01 01 01 | 01 0B 8B 61 | 03 | 01 06 06 | 0F 0F | 00 1A 1A 
50569.054758:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50569.060469: Received 016 bytes. +006615 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50570.054774:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50570.060468: Received 016 bytes. +006638 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50571.054776:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50571.060469: Received 016 bytes. +006672 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50572.054779:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50572.060469: Received 016 bytes. +006704 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50573.054775:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50573.060469: Received 016 bytes. +006742 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50573.074470: Received 018 bytes. +020167 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 08 91 | 10 10 | F9 08 02 03 
50573.080793: Received 014 bytes. +027249 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 08 92 | 0C 0C | 
50574.054774:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50574.060469: Received 016 bytes. +006779 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50574.074470: Received 018 bytes. +020202 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 08 91 | 10 10 | F9 00 03 FC 
50574.080793: Received 014 bytes. +027286 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 08 92 | 0C 0C | 
50575.054779:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50575.060469: Received 016 bytes. +006808 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50575.074468: Received 018 bytes. +020233 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 08 91 | 10 10 | F9 00 04 FD 
50575.080795: Received 014 bytes. +027317 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 08 92 | 0C 0C | 
50576.054774:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50576.060469: Received 016 bytes. +006851 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50576.074470: Received 018 bytes. +020274 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 08 91 | 10 10 | F9 00 05 FE 
50576.080795: Received 014 bytes. +027357 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 08 92 | 0C 0C | 
50577.054776:     Sent 027 bytes. +000000 μs:| 01 01 01 01 | 03 01 01 02 | 01 | 09 00 15 | 19 19 | 03 01 01 02 0B 81 03 61 91 0A 00 00 92 
50577.060469: Received 016 bytes. +006638 μs:| 01 01 01 01 | 81 03 61 91 | 05 | 09 08 90 | 0E 0E | 00 00 
50577.073797: Received 018 bytes. +020063 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 08 91 | 10 10 | F9 00 06 FF 
50577.080469: Received 014 bytes. +027145 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 08 92 | 0C 0C | 

0x0a / 0x03 collision.

81 03 62 7C sends a 0x0a packet as 01 0B 8B 61 sends a 0x03. monitor sees the 0x03 but 81 03 61 91 sees the 0x0a packet and responces. 81 03 62 7C didn't see the 0x03 because it was transmitting the 0x0a and doesn't add a slot.

187464.101373: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187464.113695: Received 017 bytes. +013861 μs:| 01 01 01 01 | 01 0B 8B 61 | 03 | 01 09 09 | 0F 0F | 00 1A 1A 
187464.120695: Received 016 bytes. +020775 μs:| 01 01 01 01 | 81 03 61 91 | 0B | 09 09 97 | 0E 0E | 10 10 
187465.101374: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187466.101375: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187467.101375: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187468.101375: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187469.101373: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187470.101375: Received 027 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 19 19 | 81 03 62 7C 0D 81 03 61 91 00 00 00 E5 
187470.114374: Received 017 bytes. +013796 μs:| 01 01 01 01 | 01 0B 8B 61 | 03 | 01 09 09 | 0F 0F | 00 1A 1A 
187471.101697: Received 032 bytes. +000000 μs:| 01 01 01 01 | 81 03 62 7C | 01 | 09 00 70 | 1E 1E | 81 03 62 7C 12 81 03 61 91 00 01 0B 8B 61 1A 00 00 FC 
187471.108374: Received 026 bytes. +007203 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 09 08 | 18 18 | 04 04 05 13 89 A9 04 1B 05 00 00 24 


Hull power alarm
9315.503565:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 08 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 F6 
9315.509384: Received 026 bytes. +006974 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
9315.523381: Received 018 bytes. +021634 μs:| 01 01 01 01 | 81 03 62 7C | 06 | 09 08 7D | 10 10 | F9 08 07 08 
9315.530382: Received 014 bytes. +028734 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
9315.535382: Received 014 bytes. +034086 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 08 7E | 0C 0C | 
9315.540382: Received 014 bytes. +039293 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 06 90 | 0C 0C | 

hull voltage alarm
0054.504067:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0054.510908: Received 026 bytes. +007924 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0054.525580: Received 018 bytes. +023089 μs:| 01 01 01 01 | 81 03 62 7C | 06 | 09 08 7D | 10 10 | F9 0C 01 06 
0054.531908: Received 014 bytes. +030140 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
0054.537582: Received 014 bytes. +035543 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 08 7E | 0C 0C | 
0054.542586: Received 014 bytes. +040796 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 06 90 | 0C 0C | 

light +3
0494.504063:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0494.510589: Received 026 bytes. +007819 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0494.525583: Received 018 bytes. +023141 μs:| 01 01 01 01 | 81 03 62 7C | 06 | 09 08 7D | 10 10 | E5 01 02 E8 
0494.531580: Received 014 bytes. +030035 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
0494.537577: Received 014 bytes. +035592 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 08 7E | 0C 0C | 
0494.542576: Received 014 bytes. +040877 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 07 91 | 0C 0C | 

light 0
0573.504065:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0573.510897: Received 026 bytes. +007837 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0573.525575: Received 018 bytes. +023100 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 07 90 | 10 10 | E5 00 07 EC 
0573.531579: Received 014 bytes. +030047 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
0573.537575: Received 014 bytes. +035523 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 09 7F | 0C 0C | 
0573.542575: Received 014 bytes. +040925 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 07 91 | 0C 0C | 

network finishes one command before dealing with another. network is trying to 07 a display change with host node not responding. host sends out an 0x0A. network continues sending 07's until timeout then sends 0x0b.
0570.504053:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0570.510897: Received 026 bytes. +007999 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 00 FF | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0570.525573: Received 018 bytes. +023244 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 00 89 | 10 10 | E5 02 05 EC 
0570.531896: Received 014 bytes. +030209 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 00 04 | 0C 0C | 
0570.537570: Received 014 bytes. +035662 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 00 76 | 0C 0C | 
0570.542583: Received 014 bytes. +041068 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 00 8A | 0C 0C | 
0571.504052:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0571.510574: Received 026 bytes. +007800 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0571.524698:     Sent 014 bytes. +023045 μs:| 01 01 01 01 | 01 01 01 01 | 0A | 09 00 1B | 0C 0C | 
0571.531573: Received 014 bytes. +030018 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
0571.537521: Received 014 bytes. +035463 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 08 7E | 0C 0C | 
0571.542574: Received 014 bytes. +040869 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 07 91 | 0C 0C | 
0571.547678:     Sent 014 bytes. +046025 μs:| 01 01 01 01 | 01 01 01 01 | 0A | 09 00 1B | 0C 0C | 
0572.504064:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0572.510897: Received 026 bytes. +008056 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 00 FF | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0572.525576: Received 018 bytes. +023068 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 00 89 | 10 10 | E5 00 06 EB 
0572.531896: Received 014 bytes. +030274 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 00 04 | 0C 0C | 
0572.537575: Received 014 bytes. +035490 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 00 76 | 0C 0C | 
0572.542575: Received 014 bytes. +040892 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 00 8A | 0C 0C | 
0573.504065:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0573.510897: Received 026 bytes. +007837 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 08 07 | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0573.525575: Received 018 bytes. +023100 μs:| 01 01 01 01 | 81 03 61 91 | 06 | 09 07 90 | 10 10 | E5 00 07 EC 
0573.531579: Received 014 bytes. +030047 μs:| 01 01 01 01 | 01 0B 8B 61 | 07 | 01 08 0C | 0C 0C | 
0573.537575: Received 014 bytes. +035523 μs:| 01 01 01 01 | 81 03 62 7C | 07 | 09 09 7F | 0C 0C | 
0573.542575: Received 014 bytes. +040925 μs:| 01 01 01 01 | 81 03 61 91 | 07 | 09 07 91 | 0C 0C | 
0574.504052:     Sent 037 bytes. +000000 μs:| 01 01 01 01 | 01 01 01 01 | 01 | 09 00 12 | 23 23 | 01 01 01 01 17 81 03 61 91 00 81 03 62 7C 00 01 0B 8B 61 1A 00 00 05 
0574.510898: Received 026 bytes. +008138 μs:| 01 01 01 01 | 01 0B 8B 61 | 02 | 01 00 FF | 18 18 | 04 04 06 13 89 AA 04 1B 05 00 00 24 
0574.532576: Received 016 bytes. +030355 μs:| 01 01 01 01 | 01 0B 8B 61 | 0B | 01 00 08 | 0E 0E | 00 00 
0574.537577: Received 016 bytes. +035575 μs:| 01 01 01 01 | 81 03 62 7C | 0B | 09 00 7A | 0E 0E | 90 90 
0574.542897: Received 016 bytes. +040975 μs:| 01 01 01 01 | 81 03 61 91 | 0B | 09 00 8E | 0E 0E | 10 10 
*/
