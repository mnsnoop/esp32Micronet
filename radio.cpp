#include "radio.h"

Radio *rRadio = NULL;

void IRAM_ATTR OutgoingTimerCallback()
{//hardware timer callback. it sends a null packet to wake up the sender thread. Probably should use another form or signaling but this was easy.
	_sPacketOutgoing pog;
	pog.p.bSize = 0;
	pog.p.tTime = 0;
	pog.p.bMessage = NULL;
	pog.iPreambleBytes = 0;
	pog.cbCallback = NULL;
	pog.vArg = NULL;
	
	xQueueSendToBackFromISR(rRadio->qhOutgoingPacket, (const void *)&pog, NULL);
	//fixme: potential issue. if this message fails to send with a full q then the timer will never get reset and send q never processed.
}

void IRAM_ATTR ISRCalibration()
{//This creates an even data signal (0x55 0b01010101) for calibrating.
	if (rRadio->bCalFlipFlop)
	{
		digitalWrite(PIN_IO, 1);
		rRadio->bCalFlipFlop = false;
	}
	else
	{
		digitalWrite(PIN_IO, 0);
		rRadio->bCalFlipFlop = true;
	}
}

void IRAM_ATTR ISRDataOut()
{//ISR triggered by the radio's clock to set the data pin
	byte b;

	if (rRadio->iBitPos < 0)
	{//get next byte.
		switch (rRadio->eOutputState)
		{
			case OS_START_PADDING_BYTE:
				rRadio->eOutputState = OS_PREAMBLE;
			break;	
			case OS_PREAMBLE:
				rRadio->iPreambleTimeout--;
				if (rRadio->iPreambleTimeout == 1)
					rRadio->eOutputState = OS_SYNCWORD;
			break;	
			case OS_SYNCWORD:
				rRadio->tSent = esp_timer_get_time(); //log start of packet for timing purposes.
				rRadio->eOutputState = OS_DATA;
			break;
			case OS_DATA:
				rRadio->iBytePos++;
				if (rRadio->iBytePos >= rRadio->iPacketSize)
					rRadio->eOutputState = OS_END_PADDING_BYTE;
			break;
			case OS_END_PADDING_BYTE:
				rRadio->Stop();
				return;
			break;
		}

		rRadio->iBitPos = 7;
	}
	
	switch (rRadio->eOutputState)
	{
		case OS_START_PADDING_BYTE:
			b = 0;
		break;	
		case OS_PREAMBLE:
			b = MN_PREAMBLE;
		break;	
		case OS_SYNCWORD:
			b = MN_SYNCWORD;
		break;
		case OS_DATA:
			b = rRadio->bWorkingBytes[rRadio->iBytePos];
		break;
		case OS_END_PADDING_BYTE:
			b = 0;
		break;
	}

	digitalWrite(PIN_IO, GetBit(b, rRadio->iBitPos--));
}

void IRAM_ATTR ISRDataIn()
{//ISR triggered by the radio's clock to read the data pin
	bool bIn = digitalRead(PIN_IO);

#ifdef CC1000
	bIn = !bIn; //*Note: When using high-side LO injection the data at DIO will be inverted.
#endif

	if (rRadio->eInputState != IS_DATA)
	{//if we're waiting for preable only save the last 4 bytes. shift the bits to the left. put the newest bit at the back.
		uint32_t *iP = (uint32_t *) rRadio->bWorkingBytes;
		*iP <<= 1;
		SetBit((byte *)&rRadio->bWorkingBytes[0], 0, bIn);
	}

	if (rRadio->eInputState == IS_WAIT &&
		(rRadio->bWorkingBytes[0] == MN_PREAMBLE &&
		 rRadio->bWorkingBytes[1] == MN_PREAMBLE &&
		 rRadio->bWorkingBytes[2] == MN_PREAMBLE &&
		 rRadio->bWorkingBytes[3] == MN_PREAMBLE))
	{ //we're getting a preamble. lock the average filter and change status.
#ifdef CC1000
		rRadio->ProgramWrite(0b00100001, 0b00011011, false);
#endif
		rRadio->eInputState = IS_PREAMBLE;
		rRadio->iPreambleTimeout = 16;
		//print("preamble(): %.2X %.2X %.2X %.2X\n", Radio->bWorkingBytes[0], Radio->bWorkingBytes[1], Radio->bWorkingBytes[2], Radio->bWorkingBytes[3]);
	}
	else if (rRadio->eInputState == IS_PREAMBLE)
	{
		if (rRadio->bWorkingBytes[0] == MN_PREAMBLE)
			rRadio->iPreambleTimeout = 16;
		else if (rRadio->bWorkingBytes[0] == MN_SYNCWORD &&
				rRadio->bWorkingBytes[1] == MN_PREAMBLE &&
				rRadio->bWorkingBytes[2] == MN_PREAMBLE &&
				rRadio->bWorkingBytes[3] == MN_PREAMBLE)
		{//end of the preamble. setup for data.
			rRadio->eInputState = IS_DATA;
			rRadio->iBitPos = 7;
			rRadio->iBytePos = 0;
			rRadio->iPacketSize = MN_MAXPACKETSIZE;
			rRadio->tPacketTime = esp_timer_get_time();
		}

		rRadio->iPreambleTimeout--;
		if (rRadio->iPreambleTimeout == 0)
		{ //This isn't the preamble we're looking for. unlock avg filter. reset.
#ifdef CC1000
			rRadio->ProgramWrite(0b00100001, 0b00001011, false); //unlock average filter
#endif
			//print("ISRDataIn(): Error, Preamble error tolerance exceeded. %.2X %.2X %.2X %.2X\n", rRadio->bWorkingBytes[0], rRadio->bWorkingBytes[1], rRadio->bWorkingBytes[2], rRadio->bWorkingBytes[3]);
			rRadio->eInputState = IS_WAIT;
			rRadio->bWorkingBytes[0] = 0;
			rRadio->bWorkingBytes[1] = 0;
			rRadio->bWorkingBytes[2] = 0;
			rRadio->bWorkingBytes[3] = 0;
		}
	}
	else if (rRadio->eInputState == IS_DATA)
	{//We're recieving a packet.
		SetBit((byte *)&rRadio->bWorkingBytes[rRadio->iBytePos], rRadio->iBitPos, bIn);
		rRadio->iBitPos--;

		if (rRadio->iBitPos < 0)
		{//current byte is filled up.
			rRadio->iBitPos = 7; //reset bitpos
			rRadio->iBytePos++; //increment bytepos

			if (rRadio->iBytePos == 14)
			{//check length checksum and see if this is a legit packet
				if (rRadio->bWorkingBytes[12] == rRadio->bWorkingBytes[13])
				{//match, set the size.
					rRadio->iPacketSize = rRadio->bWorkingBytes[13] - 11;
				}
				else
				{//they don't match. bad packet or error. unlock avg filter. reset.
#ifdef CC1000
					rRadio->ProgramWrite(0b00100001, 0b00001011, false); //unlock average filter
#endif
					//print("ISRDataIn(): Error, length bytes don't match (%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X)\n", rRadio->bWorkingBytes[0], rRadio->bWorkingBytes[1], rRadio->bWorkingBytes[2], rRadio->bWorkingBytes[3], rRadio->bWorkingBytes[4], rRadio->bWorkingBytes[5], rRadio->bWorkingBytes[6], rRadio->bWorkingBytes[7], rRadio->bWorkingBytes[8], rRadio->bWorkingBytes[9], rRadio->bWorkingBytes[10], rRadio->bWorkingBytes[11], rRadio->bWorkingBytes[12], rRadio->bWorkingBytes[13], rRadio->bWorkingBytes[14]);
					rRadio->eInputState = IS_WAIT;
				}
			}

			rRadio->iPacketSize--;
			if (rRadio->iPacketSize <= 0)
			{//received whole packet. ship it out.
				_sPacket p;
				p.bSize = rRadio->iBytePos;
				p.tTime = rRadio->tPacketTime;
				p.bMessage = (byte *)malloc(sizeof(byte) * rRadio->iBytePos);
				memcpy(p.bMessage, rRadio->bWorkingBytes, p.bSize);

				if (xQueueSendToBackFromISR(rRadio->qhIncomingPacket, (const void *)&p, NULL) != pdPASS)
				{
					free(p.bMessage);
					print(LL_ERROR, "ISRDataIn(): Error adding byte to queue.\n");
				}

				//rRadio->Stop(false);
				rRadio->iBitPos = 0;
				rRadio->eInputState = IS_WAIT;
#ifdef CC1000
				rRadio->ProgramWrite(0b00100001, 0b00001011, false); //unlock average filter
#endif
			}
		}
	}
}

Radio::Radio()
{
	eRadioState = RS_UNKNOWN;
	rRadio = this;

	thSenderBlocking = NULL;
	
	qhProgramming = xQueueCreate(QUEUEMAXPROGRAMMING, sizeof(_sProgramming));
	qhIncomingPacket = xQueueCreate(QUEUEMAXINCOMINGPACKETS, sizeof(_sPacket));
	qhOutgoingPacket = xQueueCreate(QUEUEMAXOUTGOINGPACKETS, sizeof(_sPacketOutgoing));
	
	//fixme: optimize stack sizes
	xTaskCreate(ProgramHandler, "Programming handler", configMINIMAL_STACK_SIZE + 1000, (void *)this, tskIDLE_PRIORITY + 10, &thProgramHandler);
	xTaskCreatePinnedToCore(OutgoingPacketHandler, "Outgoing packet handler", configMINIMAL_STACK_SIZE + 3000, (void *)this, tskIDLE_PRIORITY + 9, &thOutgoingPacketHandler, 1);
	
	sphProgrammingInProgress = xSemaphoreCreateBinary();
	xSemaphoreGive(sphProgrammingInProgress);

	hwtOutgoing = timerBegin(3, 80, true); //nmea2k uses hw timer 0.
	timerAttachInterrupt(hwtOutgoing, &OutgoingTimerCallback, true);

	Initialize();
}

Radio::~Radio()
{
	timerEnd(hwtOutgoing);
	
	vSemaphoreDelete(sphProgrammingInProgress);

	vTaskDelete(thProgramHandler);
	vTaskDelete(thOutgoingPacketHandler);

	vQueueDelete(qhOutgoingPacket);
	vQueueDelete(qhIncomingPacket);
	vQueueDelete(qhProgramming);

#ifdef CC1101
	SPI.endTransaction();
	SPI.end();
#endif

	rRadio = NULL;
}

void Radio::ProgramHandler(void *c)
{
	Radio *ccR = (Radio *)c;
	for (;;)
	{
		_sProgramming sProgramming;

		if (xQueueReceive(ccR->qhProgramming, (void *)&sProgramming, portMAX_DELAY) == pdFALSE)
		{
			print(LL_ERROR, "Radio::ProgramHandler(): pdFalse returned from xQueueReceive while waiting for sProgramming.\n");
			continue;
		}

		xSemaphoreTake(ccR->sphProgrammingInProgress, portMAX_DELAY);

#ifdef CC1000
		pinMode(PIN_PDATA, OUTPUT);
		digitalWrite(PIN_PDATA, 1);
		digitalWrite(PIN_PCLOCK, 1);
		digitalWrite(PIN_PALE, 1);

		ets_delay_us(3);
		digitalWrite(PIN_PALE, 0);
		ets_delay_us(2);

		for (int i = 7; i >= 0; i--)
		{ //write address
			digitalWrite(PIN_PCLOCK, 1);
			digitalWrite(PIN_PDATA, GetBit(sProgramming.bAddress, i));
			ndelay();

			digitalWrite(PIN_PCLOCK, 0);
			ndelay();
			ndelay();
		}

		digitalWrite(PIN_PCLOCK, 1);
		ets_delay_us(5);

		digitalWrite(PIN_PALE, 1);
		ets_delay_us(2);

		for (int i = 7; i >= 0; i--)
		{ //write data
			digitalWrite(PIN_PCLOCK, 1);
			digitalWrite(PIN_PDATA, GetBit(sProgramming.bData, i));
			ndelay();

			digitalWrite(PIN_PCLOCK, 0);
			ndelay();
			ndelay();
		}

		digitalWrite(PIN_PCLOCK, 1);
		ets_delay_us(5);

		digitalWrite(PIN_PDATA, 0);
		digitalWrite(PIN_PCLOCK, 0);
		digitalWrite(PIN_PALE, 1);
		ets_delay_us(5);
#endif
#ifdef CC1101
		ccR->SPI.beginTransaction(ccR->spiSettings);
		digitalWrite(PIN_CS, LOW);
		if (sProgramming.bStrobe)
		{
			ccR->SPI.transfer(sProgramming.bAddress | CC1101_WRITE_SINGLE);
		}
		else
		{
			ccR->SPI.transfer(sProgramming.bAddress | CC1101_WRITE_SINGLE);
			ccR->SPI.transfer(sProgramming.bData);
		}
		digitalWrite(PIN_CS, HIGH);
		ccR->SPI.endTransaction();
#endif

		xSemaphoreGive(ccR->sphProgrammingInProgress);

		if (sProgramming.tNotify)
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			vTaskNotifyGiveFromISR(sProgramming.tNotify, &xHigherPriorityTaskWoken);
			if (xHigherPriorityTaskWoken == pdTRUE)
				portYIELD_FROM_ISR();
		}
	}
}

void Radio::ProgramWrite(byte bAddress, byte bData, bool bBlock, bool bStrobe)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	_sProgramming sProgramming;
	sProgramming.bStrobe = bStrobe;
	sProgramming.bAddress = bAddress;
	sProgramming.bData = bData;
	if (bBlock)
		sProgramming.tNotify = xTaskGetCurrentTaskHandle();
	else
		sProgramming.tNotify = NULL;

	if (xQueueSendToBackFromISR(qhProgramming, (const void *)&sProgramming, &xHigherPriorityTaskWoken) != pdPASS)
		print(LL_ERROR, "Radio::ProgramWrite(): Error adding address byte to queue.\n");

	if (xHigherPriorityTaskWoken == pdTRUE)
		portYIELD_FROM_ISR();

	if (bBlock)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

byte Radio::ProgramRead(byte bAddress)
{
	byte ret = 0;

	xSemaphoreTake(sphProgrammingInProgress, portMAX_DELAY);

#ifdef CC1000
	pinMode(PIN_PDATA, OUTPUT);
	digitalWrite(PIN_PDATA, 1);
	digitalWrite(PIN_PCLOCK, 1);
	digitalWrite(PIN_PALE, 1);

	ets_delay_us(3);
	digitalWrite(PIN_PALE, 0);
	ets_delay_us(2);

	for (int i = 7; i >= 0; i--)
	{
		digitalWrite(PIN_PCLOCK, 1);
		digitalWrite(PIN_PDATA, GetBit(bAddress, i));
		ndelay();

		digitalWrite(PIN_PCLOCK, 0);
		ndelay();
		ndelay();
	}

	digitalWrite(PIN_PCLOCK, 1);
	ets_delay_us(5);

	digitalWrite(PIN_PDATA, 1);
	pinMode(PIN_PDATA, INPUT);
	digitalWrite(PIN_PALE, 1);
	ets_delay_us(2);

	for (int i = 7; i >= 0; i--)
	{
		if (digitalRead(PIN_PDATA))
			ret |= 1UL << i;

		digitalWrite(PIN_PCLOCK, 0);
		ndelay();
		ndelay();

		digitalWrite(PIN_PCLOCK, 1);
		ndelay();
	}

	digitalWrite(PIN_PCLOCK, 1);
	ets_delay_us(5);

	pinMode(PIN_PDATA, OUTPUT);
	digitalWrite(PIN_PDATA, 0);
	digitalWrite(PIN_PCLOCK, 0);
	digitalWrite(PIN_PALE, 1);
	ets_delay_us(5);
#endif

#ifdef CC1101
	SPI.beginTransaction(spiSettings);
	digitalWrite(PIN_CS, LOW);
	while(digitalRead(PIN_MISO))
		delay(1);
	SPI.transfer(bAddress | CC1101_READ_SINGLE);
	ret = SPI.transfer(0);
	digitalWrite(PIN_CS, HIGH);
	SPI.endTransaction();
#endif

	xSemaphoreGive(sphProgrammingInProgress);
	return ret;
}

void Radio::Initialize()
{
	pinMode(PIN_OP, OUTPUT);

#ifdef CC1000
	pinMode(PIN_PALE, OUTPUT);
	pinMode(PIN_PDATA, OUTPUT);
	pinMode(PIN_PCLOCK, OUTPUT);
	pinMode(PIN_DCLOCK, INPUT);
	pinMode(PIN_DDATA, INPUT);

	//Master reset
	digitalWrite(PIN_PALE, 1);
	digitalWrite(PIN_PDATA, 0);
	digitalWrite(PIN_PCLOCK, 0);

	ProgramWrite(0b00000001, 0b00111111, true);
	delay(2);
	ProgramWrite(0b00000001, 0b00111110, true);
	delay(2);
	ProgramWrite(0b00000001, 0b00111111, true);
	delay(2);
	//end master reset

	ProgramWrite(0b00000011, 0b01111100, false); //FreqWordA2:8151040
	ProgramWrite(0b00000101, 0b00100000, false); //FreqWordA1:8134656
	ProgramWrite(0b00000111, 0b00000000, false); //FreqWordA0:8134656
	ProgramWrite(0b00001001, 0b01011101, false); //FreqWordB2:6103040
	ProgramWrite(0b00001011, 0b00001011, false); //FreqWordB1:6097664
	ProgramWrite(0b00001101, 0b01000011, false); //FreqWordB0:6097731
	ProgramWrite(0b00001111, 0b00000001, false); //FSep1 Word: 427 Value: 64050/48037.5 Hz (refdiv 6/8)
	ProgramWrite(0b00010001, 0b10101011, false); //FSep1 Word: 427 Value: 64050/48037.5 Hz (refdiv 6/8)
	ProgramWrite(0b00010011, 0b10001100, false); //Current Register VCOCurrent: 1450µA LODrive:2mA PADrive:1mA
	ProgramWrite(0b00010101, 0b00110010, false); //Front_end Register
	ProgramWrite(0b00010111, 0b11111111, false); //PAPow 11111111
	ProgramWrite(0b00011001, 0b00110000, false); //Referance Divider: 6
	ProgramWrite(0b00011011, 0b00010000, false); //Lock Register
	ProgramWrite(0b00011101, 0b00100110, false); //Calibration Register Cal_Start: 0
	ProgramWrite(0b00011111, 0b11000011, false); //Modem2 Register PEAK_LEVEL_OFFSET:67(1000011)
	ProgramWrite(0b00100001, 0b00001011, false); //Modem1 Register LOCK_AVG_IN:0 LOCK_AVG_MODE:1 SETTLING 01 MODEM_RESET_N 1
	ProgramWrite(0b00100011, 0b01010000, false); //Modem0 Register BAUDRATE:101 DATA_FORMAT:00 XOSC_FREQ 00
	ProgramWrite(0b00100101, 0b00010000, false); //Match Register
	ProgramWrite(0b00100111, 0b00000001, false); //FSCTRL Register
	ProgramWrite(0b00111001, 0b00000000, false); //Prescaler Register
	ProgramWrite(0b10000101, 0b00111111, false); //TEST4 Register

	Calibrate();
#endif

#ifdef CC1101
	pinMode(PIN_SCLK, OUTPUT);
	pinMode(PIN_MOSI, OUTPUT);
	pinMode(PIN_MISO, INPUT);
	pinMode(PIN_CS, OUTPUT);
	pinMode(PIN_G0, INPUT); //tx/rx data

	//master reset
	digitalWrite(PIN_SCLK, HIGH);
	digitalWrite(PIN_MOSI, LOW);
	digitalWrite(PIN_CS, LOW);
	delay(1);
	digitalWrite(PIN_CS, HIGH);
	delay(1);
	digitalWrite(PIN_CS, LOW);
	while(digitalRead(PIN_MISO))
		delay(1);
	digitalWrite(PIN_CS, HIGH);

	SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, -1);
	SPI.beginTransaction(spiSettings);
	digitalWrite(PIN_CS, LOW);
	SPI.transfer(CC1101_SRES);
	while(digitalRead(PIN_MISO))
		delay(1);
	digitalWrite(PIN_CS, HIGH);
	SPI.endTransaction(); 
	//end master reset

//	ProgramWrite(CC1101_IOCFG2, 0x0B);
	ProgramWrite(CC1101_IOCFG1, 0x0B);
	ProgramWrite(CC1101_IOCFG0, 0x0C); 
//	ProgramWrite(CC1101_FIFOTHR, 0x00);
//	ProgramWrite(CC1101_SYNC1, 0x00);
//	ProgramWrite(CC1101_SYNC0, 0x00);
//	ProgramWrite(CC1101_PKTLEN, 0x00);
//	ProgramWrite(CC1101_PKTCTRL1, 0x00);
	ProgramWrite(CC1101_PKTCTRL0, 0x12); //Serial data in/out. Inf packet mode.
//	ProgramWrite(CC1101_ADDR, 0x00);
//	ProgramWrite(CC1101_CHANNR, 0x00);
	ProgramWrite(CC1101_FSCTRL1, 0x0B);
	ProgramWrite(CC1101_FSCTRL0, 0x00);
	ProgramWrite(CC1101_FREQ2, CC1101_FREQWORD2);
	ProgramWrite(CC1101_FREQ1, CC1101_FREQWORD1);
	ProgramWrite(CC1101_FREQ0, CC1101_FREQWORD0);
	ProgramWrite(CC1101_MDMCFG4, 0x7B);
	ProgramWrite(CC1101_MDMCFG3, 0x83);
	ProgramWrite(CC1101_MDMCFG2, 0x00);
	ProgramWrite(CC1101_MDMCFG1, 0x22);
	ProgramWrite(CC1101_MDMCFG0, 0xF8);
	ProgramWrite(CC1101_DEVIATN, 0x42);
//	ProgramWrite(CC1101_MCSM2, 0x00);
//	ProgramWrite(CC1101_MCSM1, 0x00);
	ProgramWrite(CC1101_MCSM0, 0x18); //0x18 = automatic calibration?!
	ProgramWrite(CC1101_FOCCFG, 0x1D);
	ProgramWrite(CC1101_BSCFG, 0x1C);
	ProgramWrite(CC1101_AGCCTRL2, 0xC7);
	ProgramWrite(CC1101_AGCCTRL1, 0x00);
	ProgramWrite(CC1101_AGCCTRL0, 0xB2);
//	ProgramWrite(CC1101_WOREVT1, 0x00);
//	ProgramWrite(CC1101_WOREVT0, 0x00);
//	ProgramWrite(CC1101_WORCTRL, 0x00);
//	ProgramWrite(CC1101_FREND1, 0x00);
	ProgramWrite(CC1101_FREND0, 0x10);
	ProgramWrite(CC1101_FSCAL3, 0xEA);
	ProgramWrite(CC1101_FSCAL2, 0x2A);
	ProgramWrite(CC1101_FSCAL1, 0x00);
	ProgramWrite(CC1101_FSCAL0, 0x1F);
//	ProgramWrite(CC1101_RCCTRL1, 0x00);
//	ProgramWrite(CC1101_RCCTRL0, 0x00);
	ProgramWrite(CC1101_FSTEST, 0x59);
//	ProgramWrite(CC1101_PTEST, 0x00);
//	ProgramWrite(CC1101_AGCTEST, 0x00);
	ProgramWrite(CC1101_TEST2, 0x81);
	ProgramWrite(CC1101_TEST1, 0x35);
	ProgramWrite(CC1101_TEST0, 0x09);

	ProgramWrite(CC1101_PATABLE, CC1101_POWER);
#endif


//	pinMode(PIN_G0, OUTPUT); //Set data pin mode to output
//	attachInterrupt(digitalPinToInterrupt(PIN_MISO), ISRCalibration, RISING); //Set special calibration interript handler
//	rRadio->ProgramStrobe(CC1101_STX, false);


	eRadioState = RS_IDLE;
}

void Radio::Calibrate()
{//fixme: this should calibrate the cc1101 too so it doesn't have such a large delay switching from rx/tx
	if (eRadioState != RS_IDLE &&
			eRadioState != RS_UNKNOWN)
	{
		print(LL_ERROR, "Radio::Calibrate(): Tried to calibrate radio when not idle (%d).\n", eRadioState);
		return;
	}

	eRadioState = RS_CALIBRATE;

	//RX calibration / Freq A
	ProgramWrite(0b00010111, 0b00000000, false); //PAPow 00000000
	ProgramWrite(0b00000001, 0b00111011, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:0 Bias_PD:1)
	ProgramWrite(0b00000001, 0b00111001, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:0 Bias_PD:0)
	ProgramWrite(0b00100001, 0b00001011, false); //Modem1 Register LOCK_AVG_IN:0 LOCK_AVG_MODE:1 SETTLING 01 MODEM_RESET_N 1
	ProgramWrite(0b00000001, 0b00110001, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0)
	ProgramWrite(0b00011001, 0b01000000, false); //Referance Divider: 8
	ProgramWrite(0b00010011, 0b10001100, false); //Current Register VCOCurrent: 1450µA LODrive:2mA PADrive:1mA
	ProgramWrite(0b00000001, 0b00010001, true); //Freq:A Power State:(RX_PD:0 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0) !!START RX!!
	ProgramWrite(0b00011101, 0b10100110, true); //Calibration Register Cal_Start: 1
	while (GetBit(ProgramRead(0b00011100), 3) == 0) //Wait for calibration to finish
		delay(1);
	ProgramWrite(0b00011101, 0b00100110, false); //Calibration Register Cal_Start: 0
	ProgramWrite(0b00000001, 0b00111001, true); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:0 Bias_PD:0) !!STOP RX!!

	//TX calibration / Freq B
	pinMode(PIN_DDATA, OUTPUT); //Set data pin mode to output
	attachInterrupt(digitalPinToInterrupt(PIN_DCLOCK), ISRCalibration, RISING); //Set special calibration interript handler
	ProgramWrite(0b00000001, 0b11110001, false); //Freq:B Power State:(RX_PD:1 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0)
	ProgramWrite(0b00011001, 0b00110000, false); //Referance Divider: 6
	ProgramWrite(0b00010011, 0b11110011, false); //Current Register VCOCurrent: 1450µA LODrive:0.5mA PADrive:4mA
	ProgramWrite(0b00000001, 0b11100001, true); //Freq:B Power State:(RX_PD:1 TX_PD:0 FS_PD:0 Core_PD:0 Bias_PD:0) !!START TX!!
	ProgramWrite(0b00011101, 0b10100110, true); //Calibration Register Cal_Start: 1
	while (GetBit(ProgramRead(0b00011100), 3) == 0) //Wait for calibration to finish
		delay(1);
	ProgramWrite(0b00011101, 0b00100110, false); //Calibration Register Cal_Start: 0
	ProgramWrite(0b00000001, 0b00111111, true); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:1 Bias_PD:1) !!STOP TX!!
	detachInterrupt(digitalPinToInterrupt(PIN_DCLOCK)); //Remove calibration interrupt handler

	eRadioState = RS_IDLE;
}

void Radio::OutgoingPacketHandler(void *r)
{
	Radio *rRadio = (Radio *)r;
	_sPacketOutgoing pog[QUEUEMAXOUTGOINGPACKETS + 1];
	bool bSkipWait = false;

	for (int i = 0; i < QUEUEMAXOUTGOINGPACKETS + 1; i++)
		pog[i].p.tTime = 0;

	for (;;)
	{
		if (bSkipWait)
			bSkipWait = false;
		else
		{
			_sPacketOutgoing pTmp;
			int ret = xQueuePeek(rRadio->qhOutgoingPacket, (void *)&pTmp, portMAX_DELAY); //If the queue is full it'll cause a race condition until not full
		}

		//disable timer incase this was manually triggered
		timerAlarmDisable(rRadio->hwtOutgoing);

		//check queue to see if there's something to send coming up
		uint64_t tCurrentTick = esp_timer_get_time();
		int tTicksTillNextSend = (int)(pog[0].p.tTime - tCurrentTick);
		bool bShift = false;
		bool bSent = false;

		if (pog[0].p.tTime != 0)
		{//next packet has a valid send time
			if (tTicksTillNextSend < -500) //missed the window. should not happen.
			{//missed the window
				print(LL_ERROR, "Radio::OutgoingPacketHandler: Missed send window by %d μs.\n", (uint32_t)tCurrentTick - pog[0].p.tTime);
				bShift = true;
			}
			else if (tTicksTillNextSend < 500)
			{//window imminent.
				if (rRadio->eRadioState != RS_IDLE)
					rRadio->Stop();
					
				while (esp_timer_get_time() < pog[0].p.tTime - 1)
					ets_delay_us(0);
	
				rRadio->eRadioState = RS_TX;
	
				rRadio->iBitPos = 7;
				rRadio->iBytePos = 0;
				rRadio->iPreambleTimeout = pog[0].iPreambleBytes;
				rRadio->iPacketSize = pog[0].p.bSize;
				rRadio->eOutputState = OS_START_PADDING_BYTE;
				rRadio->thSenderBlocking = xTaskGetCurrentTaskHandle();
				memcpy(rRadio->bWorkingBytes, pog[0].p.bMessage, pog[0].p.bSize);
	
#ifdef CC1000
				pinMode(PIN_DDATA, OUTPUT);
				attachInterrupt(digitalPinToInterrupt(PIN_DCLOCK), ISRDataOut, RISING); //Data is clocked into CC1000 at the rising edge of DCLK so we set the bit on the falling edge. 
			
				//rRadio->ProgramWrite(0b00010111, 0b00000000, false); //PAPow 00000000
				rRadio->ProgramWrite(0b00000001, 0b11110001, false); //Freq:B Power State:(RX_PD:1 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0)
				rRadio->ProgramWrite(0b00011001, 0b00110000, false); //Referance Divider: 6
				rRadio->ProgramWrite(0b00010011, 0b11110011, false); //Current Register VCOCurrent: 1450µA LODrive:0.5mA PADrive:4mA
				rRadio->ProgramWrite(0b00010111, 0b11111111, false); //PAPow 11111111
				rRadio->ProgramWrite(0b00000001, 0b11100001, false); //Freq:B Power State:(RX_PD:1 TX_PD:0 FS_PD:0 Core_PD:0 Bias_PD:0) !!START TX!!
#endif
#ifdef CC1101
				pinMode(PIN_G0, OUTPUT);
				attachInterrupt(digitalPinToInterrupt(PIN_MISO), ISRDataOut, RISING); //Data is clocked into CC1000 at the rising edge of DCLK so we set the bit on the falling edge. 
			
				rRadio->ProgramStrobe(CC1101_STX, false);
#endif

				ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait until send is finished.
				pog[0].p.tTime = rRadio->tSent; //update with actual SoP send time.
				bSent = true;
				bShift = true;
			}

			//shift the queue left
			if (bShift)
			{
				if (pog[0].cbCallback)
					pog[0].cbCallback(pog[0].vArg, &pog[0].p, bSent);
	
				if (pog[0].p.bMessage)
					free(pog[0].p.bMessage);
					
				for (int i = 0; i < QUEUEMAXOUTGOINGPACKETS; i++)
					pog[i] = pog[i + 1];
			}
		}

		//Find first free slot in q
		int i = 0;
		while (i < QUEUEMAXOUTGOINGPACKETS && pog[i].p.tTime != 0)
			i++;

		//fill the q
		while (i < QUEUEMAXOUTGOINGPACKETS && xQueueReceive(rRadio->qhOutgoingPacket, (void *)&pog[i], 0) == pdTRUE)
			i++;

		//sort the q
		bool bSwapped = false;
		i = 0;
		while (i < QUEUEMAXOUTGOINGPACKETS && pog[i + 1].p.tTime != 0)
		{
			if (pog[i + 1].p.tTime < pog[i].p.tTime)
			{
				_sPacketOutgoing ptmp = pog[i];
				pog[i] = pog[i + 1];
				pog[i + 1] = ptmp;

				bSwapped = true;
			}

			i++;
			
			if (pog[i + 1].p.tTime == 0 && bSwapped == true)
			{
				i = 0;
				bSwapped = false;
			}
		}
		
		//setup timer
		if (pog[0].p.tTime > 0)
		{
			int64_t tDelay = pog[0].p.tTime - esp_timer_get_time() - 500;
			if (tDelay < 0)
			{
				bSkipWait = true;
//				print(LL_DEBUG, "Skipping delay. pog[0].p.tTime = %d gettime() = %d\n", (uint32_t)pog[0].p.tTime, (uint32_t)esp_timer_get_time());
			}
			else
			{
//				print(LL_DEBUG, "Setting delay to %dμs. pog[0].p.tTime = %d gettime() = %d\n", (uint32_t)tDelay, (uint32_t)esp_timer_get_time(), (uint32_t)esp_timer_get_time());
				
				timerWrite(rRadio->hwtOutgoing, 0);
				timerAlarmWrite(rRadio->hwtOutgoing, tDelay, false);
				timerAlarmEnable(rRadio->hwtOutgoing);
			}
		}
	}
}

void Radio::Stop()
{
	if (eRadioState == RS_UNKNOWN)
	{
		print(LL_ERROR, "CCRadio::Stop(): Tried to stop radio in unknown state.\n");
		return;
	}

#ifdef CC1000
	detachInterrupt(digitalPinToInterrupt(PIN_DCLOCK));
	ProgramWrite(0b00000001, 0b00111111, bBlock); //idle radio
	ProgramWrite(0b00010111, 0b00000000, bBlock); //PAPow 00000000 Not sure why but the Micronet MCU's do this after idling the radio.
#endif
#ifdef CC1101
	detachInterrupt(digitalPinToInterrupt(PIN_MISO));
	ProgramStrobe(CC1101_SIDLE);
#endif

	if (thSenderBlocking)
	{
		xTaskNotifyGive(thSenderBlocking);
		thSenderBlocking = NULL;
	}

	eRadioState = RS_IDLE;
}

void Radio::Listen()
{
	if (eRadioState != RS_IDLE)
	{
		print(LL_ERROR, "Radio::Listen(): Tried to RX when not idle (%d).\n", eRadioState);
		return;
	}

	//setup for recieving data
	memset(bWorkingBytes, 0, MN_MAXPACKETSIZE);
	iBitPos = 0;
	eInputState = IS_WAIT;

#ifdef CC1000
	ProgramWrite(0b00010111, 0b00000000, false); //PAPow 00000000
	ProgramWrite(0b00000001, 0b00111011, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:0 Bias_PD:1)
	ProgramWrite(0b00000001, 0b00111001, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:1 Core_PD:0 Bias_PD:0)
	ProgramWrite(0b00100001, 0b00001011, false); //Modem1 Register LOCK_AVG_IN:0 LOCK_AVG_MODE:1 SETTLING 01 MODEM_RESET_N 1
	ProgramWrite(0b00000001, 0b00110001, false); //Freq:A Power State:(RX_PD:1 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0)
	ProgramWrite(0b00011001, 0b01000000, false); //Referance Divider: 8
	ProgramWrite(0b00010011, 0b10001100, false); //Current Register VCOCurrent: 1450µA LODrive:2mA PADrive:1mA
	ProgramWrite(0b00000001, 0b00010001, false); //Freq:A Power State:(RX_PD:0 TX_PD:1 FS_PD:0 Core_PD:0 Bias_PD:0) !!START RX!!

	pinMode(PIN_DDATA, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_DCLOCK), ISRDataIn, FALLING); //datasheet says data should be clocked in on the rising edge. I've found falling far more stable.
#endif
#ifdef CC1101
	ProgramStrobe(CC1101_SRX, false);

	pinMode(PIN_G0, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_MISO), ISRDataIn, FALLING); //datasheet says data should be clocked in on the rising edge. Given the ~7us ISR delay I've found falling far more stable.
#endif
}

void Radio::Send(byte *bData, int iLength, uint64_t tSendAt, int iPreambleBytes, void (*cbCallback)(void *v, _sPacket *p, bool bSent), void *vArg)
{
	if (uxQueueMessagesWaiting(qhOutgoingPacket) < QUEUEMAXOUTGOINGPACKETS - 2)//the -2 leaves room for the hardware timer to send a null packet to wake the sender task.
	{
		_sPacketOutgoing pog;

		if (iLength > MN_MAXPACKETSIZE)
			iLength = MN_MAXPACKETSIZE;
			
		pog.p.bSize = iLength;
		pog.p.tTime = tSendAt;
		pog.p.bMessage = (byte *)malloc(iLength * sizeof(byte));
		memcpy(pog.p.bMessage, bData, iLength);	

		pog.iPreambleBytes = iPreambleBytes + 1; //fixme why the +1? needed???
		pog.cbCallback = cbCallback;
		pog.vArg = vArg;

		xQueueSendToBack(qhOutgoingPacket, (const void *)&pog, portMAX_DELAY);
	}
}
