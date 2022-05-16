#ifndef _RADIO_H
#define _RADIO_H

#include <SPI.h>
#include "printbuffer.h"

// Select EU (868MHz) or not-EU (915MHz) Micronet frequency
// 0 -> EU, UK (868Mhz)
// 1 -> non-EU (915Mhz)
#define FREQUENCY_SYSTEM 0

#if (FREQUENCY_SYSTEM == 0)
//EU (this is most likely wrong. Someone please update with correct value)
#define CC1101_FREQWORD2	0x21
#define CC1101_FREQWORD1	0x75
#define CC1101_FREQWORD0	0x34
#endif
#if (FREQUENCY_SYSTEM == 1)
#define CC1101_FREQWORD2	0x23
#define CC1101_FREQWORD1	0x3A
#define CC1101_FREQWORD0	0x4C
#endif

#define CC1101_POWER		CC1101_PATABLE_P10
//See below for values. I found that a good CC1101 can easily overpower its output stage so you'll need to turn down the power.
//If your device is really close you might need N20.
//You can find your own sweet spot by going to setup->health->type 8 on a display and playing with the setting. You'll have to exit
//and reenter the health menu on the display each time you restart the esp32 to trigger the node info packet send.

//Set your Network ID here. Device Id can be whatever you want.
#define MN_NETWORK_ID0	0x01
#define MN_NETWORK_ID1	0x0A
#define MN_NETWORK_ID2	0xC0
#define MN_NETWORK_ID3	0x12
#define MN_DEVICE_ID0	0x01
#define MN_DEVICE_ID1	0x02
#define MN_DEVICE_ID2	0x03
#define MN_DEVICE_ID3	0x04

#define QUEUEMAXPROGRAMMING 	100	//queue size for programming commands.
#define QUEUEMAXINCOMINGPACKETS 20	//queue size for incoming packets.
#define QUEUEMAXOUTGOINGPACKETS 20	//queue size for outgoing packets.

#define MN_BYTEXMITTIME 104.17
#define MN_ANNOUNCEMENTLENGTH (int)((6 * 1000000) / MN_BYTEXMITTIME) //6 seconds of preamble. MN devices normally use 5 but we use an extra 1 since we're on bootleg hardware.
#define MN_PREAMBLELEN 13 //Micronet devices start their data bursts with a 0x00 byte, 13 bytes of 0x55, a 0x99 syncword, the data, then another 0x00 byte.
#define MN_PREAMBLE 0x55
#define MN_SYNCWORD 0x99

#define MN_MAXPACKETSIZE	200	//protocol max is 255 but devices won't process anything over 200.
#define MN_MAXNODES			32	//protocol limited by sync packet size.
#define MN_PINGINTERVALS 	60	//how long between pings in seconds.

#define CC1101_WRITE_SINGLE 0x00
#define CC1101_WRITE_BURST 0x40
#define CC1101_READ_SINGLE 0x80
#define CC1101_READ_BURST 0xC0

// CC1101 CONFIG REGSITER
#define CC1101_IOCFG2       0x00        // GDO2 output pin configuration
#define CC1101_IOCFG1       0x01        // GDO1 output pin configuration
#define CC1101_IOCFG0       0x02        // GDO0 output pin configuration
#define CC1101_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1        0x04        // Sync word, high INT8U
#define CC1101_SYNC0        0x05        // Sync word, low INT8U
#define CC1101_PKTLEN       0x06        // Packet length
#define CC1101_PKTCTRL1     0x07        // Packet automation control
#define CC1101_PKTCTRL0     0x08        // Packet automation control
#define CC1101_ADDR         0x09        // Device address
#define CC1101_CHANNR       0x0A        // Channel number
#define CC1101_FSCTRL1      0x0B        // Frequency synthesizer control
#define CC1101_FSCTRL0      0x0C        // Frequency synthesizer control
#define CC1101_FREQ2        0x0D        // Frequency control word, high INT8U
#define CC1101_FREQ1        0x0E        // Frequency control word, middle INT8U
#define CC1101_FREQ0        0x0F        // Frequency control word, low INT8U
#define CC1101_MDMCFG4      0x10        // Modem configuration
#define CC1101_MDMCFG3      0x11        // Modem configuration
#define CC1101_MDMCFG2      0x12        // Modem configuration
#define CC1101_MDMCFG1      0x13        // Modem configuration
#define CC1101_MDMCFG0      0x14        // Modem configuration
#define CC1101_DEVIATN      0x15        // Modem deviation setting
#define CC1101_MCSM2        0x16        // Main Radio Control State Machine configuration
#define CC1101_MCSM1        0x17        // Main Radio Control State Machine configuration
#define CC1101_MCSM0        0x18        // Main Radio Control State Machine configuration
#define CC1101_FOCCFG       0x19        // Frequency Offset Compensation configuration
#define CC1101_BSCFG        0x1A        // Bit Synchronization configuration
#define CC1101_AGCCTRL2     0x1B        // AGC control
#define CC1101_AGCCTRL1     0x1C        // AGC control
#define CC1101_AGCCTRL0     0x1D        // AGC control
#define CC1101_WOREVT1      0x1E        // High INT8U Event 0 timeout
#define CC1101_WOREVT0      0x1F        // Low INT8U Event 0 timeout
#define CC1101_WORCTRL      0x20        // Wake On Radio control
#define CC1101_FREND1       0x21        // Front end RX configuration
#define CC1101_FREND0       0x22        // Front end TX configuration
#define CC1101_FSCAL3       0x23        // Frequency synthesizer calibration
#define CC1101_FSCAL2       0x24        // Frequency synthesizer calibration
#define CC1101_FSCAL1       0x25        // Frequency synthesizer calibration
#define CC1101_FSCAL0       0x26        // Frequency synthesizer calibration
#define CC1101_RCCTRL1      0x27        // RC oscillator configuration
#define CC1101_RCCTRL0      0x28        // RC oscillator configuration
#define CC1101_FSTEST       0x29        // Frequency synthesizer calibration control
#define CC1101_PTEST        0x2A        // Production test
#define CC1101_AGCTEST      0x2B        // AGC test
#define CC1101_TEST2        0x2C        // Various test settings
#define CC1101_TEST1        0x2D        // Various test settings
#define CC1101_TEST0        0x2E        // Various test settings

//CC1101 Strobe commands
#define CC1101_SRES         0x30        // Reset chip.
#define CC1101_SFSTXON      0x31        // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
// If in RX/TX: Go to a wait state where only the synthesizer is
// running (for quick RX / TX turnaround).
#define CC1101_SXOFF        0x32        // Turn off crystal oscillator.
#define CC1101_SCAL         0x33        // Calibrate frequency synthesizer and turn it off
// (enables quick start).
#define CC1101_SRX          0x34        // Enable RX. Perform calibration first if coming from IDLE and
// MCSM0.FS_AUTOCAL=1.
#define CC1101_STX          0x35        // In IDLE state: Enable TX. Perform calibration first if
// MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled:
// Only go to TX if channel is clear.
#define CC1101_SIDLE        0x36        // Exit RX / TX, turn off frequency synthesizer and exit
// Wake-On-Radio mode if applicable.
#define CC1101_SAFC         0x37        // Perform AFC adjustment of the frequency synthesizer
#define CC1101_SWOR         0x38        // Start automatic RX polling sequence (Wake-on-Radio)
#define CC1101_SPWD         0x39        // Enter power down mode when CSn goes high.
#define CC1101_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CC1101_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CC1101_SWORRST      0x3C        // Reset real time clock.
#define CC1101_SNOP         0x3D        // No operation. May be used to pad strobe commands to two
// INT8Us for simpler software.
//CC1101 STATUS REGSITER
#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_WORTIME1     0x36
#define CC1101_WORTIME0     0x37
#define CC1101_PKTSTATUS    0x38
#define CC1101_VCO_VC_DAC   0x39
#define CC1101_TXBYTES      0x3A
#define CC1101_RXBYTES      0x3B

//CC1101 PATABLE,TXFIFO,RXFIFO
#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F
#define CC1101_RXFIFO       0x3F

#if (FREQUENCY_SYSTEM == 0)
//CC1101 PATABLE VALUES - 868MHz
#define CC1101_PATABLE_N30  0x03
#define CC1101_PATABLE_N20  0x17
#define CC1101_PATABLE_N15  0x1D
#define CC1101_PATABLE_N10  0x26
#define CC1101_PATABLE_N6 0x37
#define CC1101_PATABLE_0  0x50
#define CC1101_PATABLE_P5 0x86
#define CC1101_PATABLE_P7 0xCD
#define CC1101_PATABLE_P10  0xC5
#define CC1101_PATABLE_P12  0xC0
#else
//CC1101 PATABLE VALUES - 915MHz
#define CC1101_PATABLE_N30  0x03
#define CC1101_PATABLE_N20  0x0E
#define CC1101_PATABLE_N15  0x1E
#define CC1101_PATABLE_N10  0x27
#define CC1101_PATABLE_N6 0x38
#define CC1101_PATABLE_0  0x8E
#define CC1101_PATABLE_P5 0x84
#define CC1101_PATABLE_P7 0xCC
#define CC1101_PATABLE_P10  0xC3
#define CC1101_PATABLE_P11  0xC0
#endif

#ifdef CC1000
#define PIN_IO PIN_DDATA
#endif
#ifdef CC1101
#define PIN_IO PIN_G0
#endif

enum _eRadioState
{
	RS_UNKNOWN = 0,
	RS_CALIBRATE,
	RS_IDLE,
	RS_RX,
	RS_TX
};

enum _eInputState
{
	IS_WAIT = 0,
	IS_PREAMBLE,
	IS_DATA
};

enum _eOutputState
{
	OS_START_PADDING_BYTE = 0,
	OS_PREAMBLE,
	OS_SYNCWORD,
	OS_DATA,
	OS_END_PADDING_BYTE
};

void inline ndelay()
{
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
}

bool inline GetBit(byte b, byte pos) //0 = LSB, 7 = MSB
{
	return (b & (1 << pos)) != 0;
}

void inline SetBit(byte *b, unsigned char pos, bool val) //0 = LSB, 7 = MSB
{
	if (val)
	{
		*b |= (1UL << pos);
	}
	else
	{
		*b &= ~(1UL << pos);
	}
}

struct _sProgramming
{
	bool bStrobe;
	byte bAddress;
	byte bData;
	TaskHandle_t tNotify;
};

struct _sPacket
{
	byte bSize;
	uint64_t tTime;
	byte *bMessage;
};

struct _sPacketOutgoing
{
	_sPacket p;
	int iPreambleBytes;
	void (*cbCallback)(void *v, _sPacket *p, bool bSent);
	void *vArg;
};

void IRAM_ATTR ISRDataOut();
void IRAM_ATTR ISRDataIn();
void IRAM_ATTR ISRCalibration();

class Radio
{
	private:
	_eInputState eInputState;
	_eOutputState eOutputState;

	TaskHandle_t thProgramHandler,
				 thOutgoingPacketHandler;

	QueueHandle_t qhProgramming,
				  qhIncomingPacket,
				  qhOutgoingPacket;

	SemaphoreHandle_t sphProgrammingInProgress;

	hw_timer_t *hwtOutgoing;

#ifdef CC1101
	SPIClass SPI;
	SPISettings spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE0);
#endif

	bool bCalFlipFlop;

	_eRadioState eRadioState;
	uint64_t tSent;

	byte bWorkingBytes[MN_MAXPACKETSIZE];	//incoming/outgoing packet buffer.
	int iBitPos;							//incoming/outgoing bit postion pointer
	int iBytePos;							//incoming/outgoing byte position pointer
	int iPacketSize;						//incoming/outgoing packet size counter
	int iPreambleTimeout;					//incoming/outgoing preamble quality counter/outgoing preamble position counter
	uint64_t tPacketTime;					//incoming/outgoing timestamp of when peamble ended and packet began
	TaskHandle_t thSenderBlocking;			//outgoing thread blocking mechinism.

	static void OutgoingPacketHandler(void *r); //realtime handling of outgoing packets.
	friend void OutgoingTimerCallback();

	public:
	Radio();
	~Radio();

	void Initialize();
	void ProgramWrite(byte bAddress, byte bData, bool bBlock, bool bStrobe);
	inline void ProgramWrite(byte bAddress, byte bData, bool bBlock)
	{
		ProgramWrite(bAddress, bData, bBlock, false);
	}
	inline void ProgramWrite(byte bAddress, byte bData)
	{
		ProgramWrite(bAddress, bData, false);
	}
	inline void ProgramStrobe(byte bStrobe, bool bBlock)
	{
		ProgramWrite(bStrobe, 0, bBlock, true);
	}
	inline void ProgramStrobe(byte bStrobe)
	{
		ProgramStrobe(bStrobe, false);
	}
	byte ProgramRead(byte bAddress);

	void Calibrate();

	void Stop();
	void Listen();
	void Send(byte *bData, int iLength, uint64_t tSendAt, int iPreambleBytes, void (*cbCallback)(void *v, _sPacket *p, bool bSent), void *vArg);

	QueueHandle_t *GetIncomingQueue()
	{
		return &qhIncomingPacket;
	}

	QueueHandle_t *GetOutgoingQueue()
	{
		return &qhOutgoingPacket;
	}

	TaskHandle_t GetProgramTaskHandle()
	{
		return thProgramHandler;
	}

	TaskHandle_t GetOutgoingTaskHandle()
	{
		return thOutgoingPacketHandler;
	}

	_eRadioState RadioState()
	{
		return eRadioState;
	}

	static void ProgramHandler(void *);
	friend void IRAM_ATTR ISRDataOut();
	friend void IRAM_ATTR ISRDataIn();
	friend void IRAM_ATTR ISRCalibration();
};

#endif // _RADIO_H
