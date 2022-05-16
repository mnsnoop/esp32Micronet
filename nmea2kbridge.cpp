#include <time.h>
#include "nmea2kbridge.h"

#define ESP32_CAN_RX_PIN GPIO_NUM_22
#define ESP32_CAN_TX_PIN GPIO_NUM_23

#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>

NMEA2kBridge N2kBridge;

void MNHandler(_sDataArrayIn sDataIn, void *v2);

void Speed(const tN2kMsg &N2kMsg);
void DistanceLog(const tN2kMsg &N2kMsg);
void OutsideEnvironmental(const tN2kMsg &N2kMsg);
void WaterDepth(const tN2kMsg &N2kMsg);
void WindSpeed(const tN2kMsg &N2kMsg);
void Heading(const tN2kMsg &N2kMsg);
void COGSOG(const tN2kMsg &N2kMsg);
void LatLon(const tN2kMsg &N2kMsg);
void GNSS(const tN2kMsg &N2kMsg);
void NavInfo(const tN2kMsg &N2kMsg);
void WaypointList(const tN2kMsg &N2kMsg);
void XTE(const tN2kMsg &N2kMsg);
void BatteryStatus(const tN2kMsg &N2kMsg);
void LocalOffset(const tN2kMsg &N2kMsg);
void SystemTime(const tN2kMsg &N2kMsg);

const unsigned long ulBridgeTransmitMessages[] PROGMEM = {128259L, 128275L, 130310L, 128267L, 130306L, 127250L, 129026L, 129025L, 129029L, 129284L, 129285L, 129283L, 129033L, 127513L, 127506L, 126992L, 0};

typedef struct
{
	unsigned long PGN;
	void (*Handler)(const tN2kMsg &N2kMsg); 
} tNMEA2000Handler;

tNMEA2000Handler NMEA2000Handlers[] =
{
	{128259L, &Speed},
	{128275L, &DistanceLog},
	{130310L, &OutsideEnvironmental}, //seatemp
	{128267L, &WaterDepth},
	{130306L, &WindSpeed},
	{127250L, &Heading},
	{129026L, &COGSOG},
//	{129025L, &LatLon}, //Using GNSS instead
	{129029L, &GNSS},
	{129284L, &NavInfo}, //has vmg-wp and dtw
	{129285L, &WaypointList}, //for wp name
	{129283L, &XTE},
	{127508L, &BatteryStatus},
	{129033L, &LocalOffset},
	{126992L, &SystemTime},
	{0,0}
};

void tfNMEA2K(void *p)
{
	for (;;)
	{
		NMEA2000.ParseMessages();
		delay(20);
	}	
}

void HandleNMEA2000Msg(const tN2kMsg &N2kMsg)
{
	int iHandler;
	for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
	
	if (NMEA2000Handlers[iHandler].PGN != 0)
		NMEA2000Handlers[iHandler].Handler(N2kMsg);
}

NMEA2kBridge::NMEA2kBridge()
{
}

NMEA2kBridge::~NMEA2kBridge()
{
	mNet.RemoveTimerHandler(&MNHandler);
//	NMEA2000.Close(); ??? How do you close this!	
	vTaskDelete(tNMEA2K);	
}

void NMEA2kBridge::Start()
{	
	NMEA2000.SetN2kCANSendFrameBufSize(250);
	
	NMEA2000.SetProductInformation(	"00000001",				// Manufacturer's Model serial code
									1,						// Manufacturer's product code
									"ESP32 Micronet Bridge",// Manufacturer's Model ID
									"1.0.0.0 (2021-01-01)",	// Manufacturer's Software version code
									"1.0.0.0 (2021-01-01)",	// Manufacturer's Model version
									0xff, // load equivalency - use default
									0xffff, // NMEA 2000 version - use default
									0xff);

	NMEA2000.SetDeviceInformation(	1,		// Unique number. Use e.g. Serial number.
									130,	// Device function=Computer Bridge.
									25,		// Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
									2040,	// Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
									4);

	NMEA2000.SetN2kCANSendFrameBufSize(250);
	
//	NMEA2000.SetForwardStream(&Serial);
//	NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);
//	NMEA2000.EnableForward(true); // Disable all msg forwarding to USB (=Serial)
	NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, 22); // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
	NMEA2000.ExtendTransmitMessages(ulBridgeTransmitMessages);
	NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
			
	NMEA2000.Open();

//	mNet.AddTimerHandler(&MNHandler, (void *)this);

	xTaskCreate(tfNMEA2K, "NMEA2K Worker", configMINIMAL_STACK_SIZE + 4000, NULL, tskIDLE_PRIORITY + 1, &tNMEA2K); //stack of 200 too small, 500 seems to work. 1k for safety.
}

void NMEA2kBridge::SendAppWindAngleSpeed(double dAngle, double dSpeed)
{
	tN2kMsg N2kMsg;
	SetN2kWindSpeed(N2kMsg, 1, dSpeed, dAngle, N2kWind_Apparent);
	NMEA2000.SendMsg(N2kMsg);
}

void NMEA2kBridge::SendBatteryVoltage(double dVolts)
{
	tN2kMsg N2kMsg;
	SetN2kPGN127508(N2kMsg, 0, dVolts);
	NMEA2000.SendMsg(N2kMsg);
}

void NMEA2kBridge::SendWaterSpeed(double dSpeed)
{
	tN2kMsg N2kMsg;
	SetN2kBoatSpeed(N2kMsg, 1, dSpeed);
	NMEA2000.SendMsg(N2kMsg);
}

void MNHandler(_sDataArrayIn sDataIn, void *v2)
{ //MN -> NMEA
	uint64_t tCurrentTick = esp_timer_get_time();

	if (tCurrentTick - sDataIn.tlSpeedUpdate < 1000000)
	{
		tN2kMsg N2kMsg;
		SetN2kBoatSpeed(N2kMsg, 1, (double) KnotsToms(sDataIn.iSpeed / 100.0f));
		NMEA2000.SendMsg(N2kMsg);
	}

	if (tCurrentTick - sDataIn.tlWaterTempUpdate < 1000000)
	{
		tN2kMsg N2kMsg;
		SetN2kOutsideEnvironmentalParameters(N2kMsg, 1, (double) 273.15f + (sDataIn.iWaterTemp * 2.0f));
		NMEA2000.SendMsg(N2kMsg);
	}

	if (tCurrentTick - sDataIn.tlDepthUpdate < 1000000)
	{
		tN2kMsg N2kMsg;
		SetN2kWaterDepth(N2kMsg, 1, FeettoMeters(sDataIn.iDepth / 10.0f), 0.0f); //fixme, should use the mn offset.
		NMEA2000.SendMsg(N2kMsg);
	}

	if (tCurrentTick - sDataIn.tlAppWindSpeedUpdate < 1000000 ||
		tCurrentTick - sDataIn.tlAppWindDirUpdate < 1000000)
	{
		tN2kMsg N2kMsg;
		SetN2kWindSpeed(N2kMsg, 1, sDataIn.iAppWindSpeed / 10.0f / 1.943844f, DegToRad(sDataIn.iAppWindDir), N2kWind_Apparent); //probably wrong but no eq to test
		NMEA2000.SendMsg(N2kMsg);
	}		

	if (tCurrentTick - sDataIn.tlVoltsUpdate < 1000000)
	{
		tN2kMsg N2kMsg;
		SetN2kPGN127508(N2kMsg, 0, sDataIn.iVolts / 10.0f);
		NMEA2000.SendMsg(N2kMsg);
	}
}

void Speed(const tN2kMsg &N2kMsg)
{
	unsigned char SID;
	double SOW;
	double SOG;
	tN2kSpeedWaterReferenceType SWRT;
	
	if (ParseN2kBoatSpeed(N2kMsg, SID, SOW, SOG, SWRT))
	{
		if (!N2kIsNA(SOW))
			mNet.SetSpeed(msToKnots(SOW));
	}
}

void DistanceLog(const tN2kMsg &N2kMsg)
{ //DaysSince1970, double &SecondsSinceMidnight, uint32_t &Log, uint32_t &TripLog
	unsigned short DaysSince1970;
	double SecondsSinceMidnight;
	unsigned int Log;
	unsigned int TripLog;
	
	if (ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog))
	{
		mNet.SetTripLog(MeterstoNM(TripLog), MeterstoNM(Log));	
	}
}

void OutsideEnvironmental(const tN2kMsg &N2kMsg)
{//ParseN2kOutsideEnvironmentalParameters(const tN2kMsg &N2kMsg, unsigned char &SID, double &WaterTemperature, double &OutsideAmbientAirTemperature, double &AtmosphericPressure)
	unsigned char SID; 
	double WaterTemperature; 
	double OutsideAmbientAirTemperature;
	double AtmosphericPressure;

	if (ParseN2kOutsideEnvironmentalParameters(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure))
	{
		mNet.SetWaterTemperature(WaterTemperature - 273.15f);
	}
}

void WaterDepth(const tN2kMsg &N2kMsg)
{
	unsigned char SID;
	double DepthBelowTransducer;
	double Offset;
	
	if (ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset))
		if (!N2kIsNA(DepthBelowTransducer))
			mNet.SetDepth(MeterstoFeet(DepthBelowTransducer));
}

void WindSpeed(const tN2kMsg &N2kMsg)
{ //ParseN2kWindSpeed(const tN2kMsg &N2kMsg, unsigned char &SID, double &WindSpeed, double &WindAngle, tN2kWindReference &WindReference)
	unsigned char SID;
	double WindSpeed;
	double WindAngle;
	tN2kWindReference WindReference;

	if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference))
	{
		if (!N2kIsNA(WindAngle))
			mNet.SetApparentWindDirection(RadToDeg(WindAngle)); //this might be wrong but I don't have the eq to test.
		if (!N2kIsNA(WindSpeed))
			mNet.SetApparentWindSpeed(WindSpeed * 1.943844f);
	}
}

void Heading(const tN2kMsg &N2kMsg)
{ //ParseN2kHeading(const tN2kMsg &N2kMsg, unsigned char &SID, double &Heading, double &Deviation, double &Variation, tN2kHeadingReference &ref)
	unsigned char SID;
	double Heading; 
	double Deviation;
	double Variation;
	tN2kHeadingReference ref;

	if (ParseN2kHeading(N2kMsg, SID, Heading, Deviation, Variation, ref))
		if (!N2kIsNA(Heading))
			mNet.SetHeading(RadToDeg(Heading));
}

void COGSOG(const tN2kMsg &N2kMsg)
{ //ParseN2kCOGSOGRapid(const tN2kMsg &N2kMsg, unsigned char &SID, tN2kHeadingReference &ref, double &COG, double &SOG)
	unsigned char SID;
	tN2kHeadingReference ref;
	double COG;
	double SOG;

	if (ParseN2kCOGSOGRapid(N2kMsg, SID, ref, COG, SOG))
		if (!N2kIsNA(COG) && !N2kIsNA(SOG))
			mNet.SetSOGCOG(msToKnots(SOG), RadToDeg(COG));
}

void LatLon(const tN2kMsg &N2kMsg)
{

}

void GNSS(const tN2kMsg &N2kMsg)
{ //ParseN2kGNSS(const tN2kMsg &N2kMsg, unsigned char &SID, uint16_t &DaysSince1970, double &SecondsSinceMidnight, double &Latitude, double &Longitude, double &Altitude, tN2kGNSStype &GNSStype, tN2kGNSSmethod &GNSSmethod, unsigned char &nSatellites, double &HDOP, double &PDOP, double &GeoidalSeparation, unsigned char &nReferenceStations, tN2kGNSStype &ReferenceStationType, uint16_t &ReferenceSationID, double &AgeOfCorrection
	unsigned char SID;
	uint16_t DaysSince1970;
	double SecondsSinceMidnight;
	double Latitude;
	double Longitude;
	double Altitude;
	tN2kGNSStype GNSStype;
	tN2kGNSSmethod GNSSmethod;
	unsigned char nSatellites;
	double HDOP;
	double PDOP;
	double GeoidalSeparation;
	unsigned char nReferenceStations;
	tN2kGNSStype ReferenceStationType;
	uint16_t ReferenceSationID;
	double AgeOfCorrection;
	
	if (ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod, nSatellites, HDOP, PDOP, GeoidalSeparation, nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection))
		if (!N2kIsNA(Latitude) && !N2kIsNA(Longitude))
			mNet.SetLatLong(Latitude, Longitude);
}
void NavInfo(const tN2kMsg &N2kMsg)
{ //ParseN2kNavigationInfo(const tN2kMsg &N2kMsg, unsigned char& SID, double& DistanceToWaypoint, tN2kHeadingReference& BearingReference, bool& PerpendicularCrossed, bool& ArrivalCircleEntered, tN2kDistanceCalculationType& CalculationType, double& ETATime, int16_t& ETADate, double& BearingOriginToDestinationWaypoint, double& BearingPositionToDestinationWaypoint, uint8_t& OriginWaypointNumber, uint8_t& DestinationWaypointNumber, double& DestinationLatitude, double& DestinationLongitude, double& WaypointClosingVelocity)
	unsigned char SID;
	double DistanceToWaypoint;
	tN2kHeadingReference BearingReference;
	bool PerpendicularCrossed;
	bool ArrivalCircleEntered;
	tN2kDistanceCalculationType CalculationType;
	double ETATime;
	int16_t ETADate;
	double BearingOriginToDestinationWaypoint;
	double BearingPositionToDestinationWaypoint;
	uint8_t OriginWaypointNumber;
	uint8_t DestinationWaypointNumber;
	double DestinationLatitude;
	double DestinationLongitude;
	double WaypointClosingVelocity;

	if (ParseN2kNavigationInfo(N2kMsg, SID, DistanceToWaypoint, BearingReference, PerpendicularCrossed, ArrivalCircleEntered, CalculationType, ETATime, ETADate, BearingOriginToDestinationWaypoint, BearingPositionToDestinationWaypoint, OriginWaypointNumber, DestinationWaypointNumber, DestinationLatitude, DestinationLongitude, WaypointClosingVelocity))
	{
		if (!N2kIsNA(DistanceToWaypoint))
			mNet.SetDTW(MeterstoNM(DistanceToWaypoint));

		if (!N2kIsNA(WaypointClosingVelocity))
			mNet.SetVMG_WP(msToKnots(WaypointClosingVelocity));

		if (!N2kIsNA(BearingPositionToDestinationWaypoint))
			mNet.SetBTW(RadToDeg(BearingPositionToDestinationWaypoint));
	}

}

void WaypointList(const tN2kMsg &N2kMsg)
{
//	N2kMsg.Print(&Serial);

	int Index = 0;
	uint16_t iStart = N2kMsg.Get2ByteUInt(Index);
	uint16_t iItems = N2kMsg.Get2ByteUInt(Index);
	uint16_t iWaypoints = 0;//N2kMsg.Get2ByteUInt(Index);
	uint16_t iRoute = N2kMsg.Get2ByteUInt(Index);
	
	byte bReserved1 = N2kMsg.GetByte(Index);
	byte bReserved2 = N2kMsg.GetByte(Index);
	byte bReserved3 = N2kMsg.GetByte(Index);
	
	byte bRouteNameLength = N2kMsg.GetByte(Index);
	byte bRouteNumber = N2kMsg.GetByte(Index);
	char cRouteName[255];
	N2kMsg.GetStr(cRouteName, bRouteNameLength - 2, Index);
	cRouteName[bRouteNameLength - 2] = '\0';
	char cWPName[255]; //First entry (for me) is origin. Second is the wp we're going to so I only save it's name.
	byte bReserved4 = N2kMsg.GetByte(Index);

//	print(LL_NONE, "start %d items %d Waypoints %d route %d bRouteNameLength %d routenumber %d cRouteName %s ", iStart, iItems, iWaypoints, iRoute, bRouteNameLength, bRouteNumber, cRouteName);

	for (int iItem = 0; iItem < 2; iItem++)
	{
		uint16_t iWPId = N2kMsg.Get2ByteUInt(Index);
		byte bWPNameLength = N2kMsg.GetByte(Index);
		byte bWPNumber = N2kMsg.GetByte(Index);
		N2kMsg.GetStr(cWPName, bWPNameLength - 2, Index);
		cWPName[bWPNameLength - 2] = '\0';
		double dWPLat = N2kMsg.Get4ByteDouble(1e-07, Index);
		double dWPLong = N2kMsg.Get4ByteDouble(1e-07, Index);
//		printnts(LL_NONE, "wpid %d wplen %d wpnum %d wpname %s lat %f lon %f", iWPId, bWPNameLength, bWPNumber, cWPName, dWPLat, dWPLong);
	}

	mNet.SetWPName(cWPName);

//	printnts(LL_NONE, "\n");
}

void XTE(const tN2kMsg &N2kMsg)
{ //ParseN2kPGN129283(const tN2kMsg &N2kMsg, unsigned char& SID, tN2kXTEMode& XTEMode, bool& NavigationTerminated, double& XTE)
	unsigned char SID;
	tN2kXTEMode XTEMode;
	bool NavigationTerminated;
	double XTE;
	
	if (ParseN2kPGN129283(N2kMsg, SID, XTEMode, NavigationTerminated, XTE))
	{
		if (!NavigationTerminated && !N2kIsNA(XTE))
			mNet.SetXTE(MeterstoNM(XTE));	
	}
}

void BatteryStatus(const tN2kMsg &N2kMsg)
{ //ParseN2kPGN127508(const tN2kMsg &N2kMsg, unsigned char &BatteryInstance, double &BatteryVoltage, double &BatteryCurrent, double &BatteryTemperature, unsigned char &SID)
	unsigned char BatteryInstance;
	double BatteryVoltage;
	double BatteryCurrent;
	double BatteryTemperature;
	unsigned char SID;
	
	if (ParseN2kPGN127508(N2kMsg, BatteryInstance, BatteryVoltage, BatteryCurrent, BatteryTemperature, SID))
	{
		if (BatteryInstance == 0 && !N2kIsNA(BatteryVoltage))
			mNet.SetVolts(BatteryVoltage);			
	}
}

void LocalOffset(const tN2kMsg &N2kMsg)
{
	uint16_t SystemDate;
	double SystemTime;
	int16_t Offset;
    
	if (ParseN2kLocalOffset(N2kMsg,SystemDate,SystemTime,Offset))
    {
		struct tm t;
		memset(&t, 0, sizeof(struct tm));
		t.tm_year = 70;
		t.tm_mon = 0;
		t.tm_mday = SystemDate;
		t.tm_sec = SystemTime;
//		t.tm_min = Offset;
		mktime(&t);
		/* show result */

		mNet.SetDate(t.tm_mday + 1, t.tm_mon + 1, t.tm_year); /* prints: Sun Mar 16 00:00:00 1980 */
		mNet.SetTime(t.tm_hour, t.tm_min);
	}
}

void SystemTime(const tN2kMsg &N2kMsg)
{ //ParseN2kPGN126992(const tN2kMsg &N2kMsg, unsigned char &SID, uint16_t &SystemDate, double &SystemTime, tN2kTimeSource &TimeSource)
	unsigned char SID;
	uint16_t SystemDate;
	double SystemTime;
	tN2kTimeSource TimeSource;
	
	if (ParseN2kSystemTime(N2kMsg, SID, SystemDate, SystemTime, TimeSource))
	{
		struct tm t;
		memset(&t, 0, sizeof(struct tm));
		t.tm_year = 70;
		t.tm_mon = 0;
		t.tm_mday = SystemDate;
		t.tm_sec = SystemTime;
//		t.tm_min = Offset;
		mktime(&t);
		/* show result */

		mNet.SetDate(t.tm_mday + 1, t.tm_mon + 1, t.tm_year); /* prints: Sun Mar 16 00:00:00 1980 */
		mNet.SetTime(t.tm_hour, t.tm_min);
	}
}
