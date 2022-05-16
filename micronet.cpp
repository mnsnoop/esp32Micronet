#include "micronet.h"

void IRAM_ATTR MicronetTimerCallback(TimerHandle_t xTimer);

Micronet mNet;

Micronet::Micronet()
{
	mnIdMyNetwork.bId[0] = MN_NETWORK_ID0;
	mnIdMyNetwork.bId[1] = MN_NETWORK_ID1;
	mnIdMyNetwork.bId[2] = MN_NETWORK_ID2;
	mnIdMyNetwork.bId[3] = MN_NETWORK_ID3;
	mnIdMyDevice.bId[0] = MN_DEVICE_ID0;
	mnIdMyDevice.bId[1] = MN_DEVICE_ID1;
	mnIdMyDevice.bId[2] = MN_DEVICE_ID2;
	mnIdMyDevice.bId[3] = MN_DEVICE_ID3;

	eMNStatus = MNS_Force_Node;
//	eMNStatus = MNS_NetworkChoice;
//	eMNStatus = MNS_TestMode1;

	for (int i = 0; i < 10; i++)
	{
		sIncomingPacketHandlers[i].cbCallback = NULL;
		sTimerHandlers[i].cbCallback = NULL;
	}

	for (int i = 0; i < MN_MAXNODES; i++)
	{
		sMNSync[i].mnIdDevice.bId[0] = 0;
		sMNSync[i].mnIdDevice.bId[1] = 0;
		sMNSync[i].mnIdDevice.bId[2] = 0;
		sMNSync[i].mnIdDevice.bId[3] = 0;
		sMNSync[i].bWindowSize = 0;
	}

	iMNNodes = 0;

	memset(&sDataArrayIn, 0, sizeof(sDataArrayIn));
	memset(&sDataArrayOut, 0, sizeof(sDataArrayOut));
	sDataArrayOut.bNodeInfoType = 0x08; //we're a 'type 8' device.
	sDataArrayOut.bNodeInfoVMajor = 0x01; //version 1
	sDataArrayOut.bNodeInfoVMinor = 0x00; //.0
	sDataArrayOut.bSendNodeInfo = false;

	qhIncomingPacket = CCRadio.GetIncomingQueue();
	qhOutgoingPacket = CCRadio.GetOutgoingQueue();

	xTaskCreate(MicronetWorkerThread, "Micronet Worker", configMINIMAL_STACK_SIZE + 5000, (void *)this, tskIDLE_PRIORITY + 7, &thMicronetWorker);
	xTaskCreate(IncomingPacketHandlerCB, "Incoming packet handler", configMINIMAL_STACK_SIZE + 3000, (void *)this, tskIDLE_PRIORITY + 8, &thIncomingPacketHandler);

	tiMicronet = xTimerCreate("Timer Micronet", pdMS_TO_TICKS(100), pdTRUE, (void *) 0, MicronetTimerCallback);
}

Micronet::~Micronet()
{
	xTimerDelete(tiMicronet, portMAX_DELAY);

	vTaskDelete(thMicronetWorker);
	vTaskDelete(thIncomingPacketHandler);
}

void Micronet::Start()
{
	xTimerStart(tiMicronet, 0);	
}

void Micronet::IncomingPacketHandlerCB(void *m)
{
	Micronet *mNet = (Micronet *)m;

	for (;;)
	{
		_sPacket p;

		if (xQueueReceive(*mNet->qhIncomingPacket, (void *)&p, portMAX_DELAY) == pdFALSE)
		{
			print(LL_ERROR, "Micronet::IncomingPacketHandler(): pdFalse returned from xQueueReceive while waiting for packet.\n");
			continue;
		}

		mNet->IncomingPacketHandler(&p);
	}
}

void Micronet::IncomingPacketHandler(_sPacket *p)
{
	_cMNId	mnIdPacketNetwork{p->bMessage[0], p->bMessage[1], p->bMessage[2], p->bMessage[3]},
		 	mnIdPacketDevice{p->bMessage[4], p->bMessage[5], p->bMessage[6], p->bMessage[7]};

	int iPacketTime = 0,
		iPredictedWindow = 0;
		 	
	if (GetCRC(p->bMessage, 11) != p->bMessage[11])
	{
		print(LL_ERROR, "Packet failed header CRC check. (CRC given %x expected %x)\n", p->bMessage[11], GetCRC(p->bMessage, 11));
		goto end;
	}

	if (mnIdPacketNetwork != mnIdMyNetwork)
	{
		print(LL_INFO, "Packet isn't for our network.\n");
		goto end;
	}

	if ((eMNStatus == MNS_NetworkChoice_S1 || eMNStatus == MNS_NetworkChoice_S2) && p->bMessage[8] == 0x01)
	{//network already running, switching to node mode.
		print(LL_INFO, "Network choice mode: Network detected, switching to node mode.\n");
		eMNStatus = MNS_NetworkChoice_N1;
	}

	if ((eMNStatus == MNS_NetworkChoice_N1 || eMNStatus == MNS_Force_Node_Listening || 
		eMNStatus == MNS_NetworkChoice_Node || eMNStatus == MNS_Force_Node_Connected) && p->bMessage[8] == 0x01)
	{
		if (GetCRC(p->bMessage + 14, p->bSize - 15) != p->bMessage[p->bSize - 1])
		{
			print(LL_ERROR, "Packet failed data CRC check. (CRC given %x expected %x)\n", p->bMessage[p->bSize - 1], GetCRC(p->bMessage + 14, p->bSize - 15));
			goto end;
		}

		int iNodes = 0;
		for (int i = 14; i < p->bSize - 3; i += 5)
		{
			sMNSync[iNodes].mnIdDevice.bId[0] = p->bMessage[i];
			sMNSync[iNodes].mnIdDevice.bId[1] = p->bMessage[i + 1];
			sMNSync[iNodes].mnIdDevice.bId[2] = p->bMessage[i + 2];
			sMNSync[iNodes].mnIdDevice.bId[3] = p->bMessage[i + 3];
			sMNSync[iNodes].bWindowSize = p->bMessage[i + 4];
			iNodes++;	
		}
		
		iMNNodes = iNodes;
		tLAnnounce = p->tTime;
		iSyncPacketOffset = p->tTime - (uint64_t)((uint32_t)(p->tTime / 1000000) * 1000000);

//		for (int i = 0; i < iMNNodes; i++)
//			print(LL_INFO, "%.2X:%.2X:%.2X:%.2X - %.2X\n", sMNSync[i].nmIdDevice.bId[0], sMNSync[i].nmIdDevice.bId[1], sMNSync[i].nmIdDevice.bId[2], sMNSync[i].nmIdDevice.bId[3], sMNSync[i].bWindowSize);	
	}

	iPacketTime = p->tTime - tLAnnounce;
	iPredictedWindow = GetPacketWindow(mnIdPacketDevice, p->bMessage[8]);

	if (p->bMessage[8] == 0x01)
	{
		if (eMNStatus == MNS_NetworkChoice_Node || eMNStatus == MNS_Force_Node_Connected)
		{
			int iPlace = FindMNDeviceInSyncList(mnIdMyDevice);
			if (iPlace == -1)
			{ //we've been disconnected
				if (eMNStatus == MNS_NetworkChoice_Node)
					eMNStatus = MNS_NetworkChoice_N1;
				else if (eMNStatus == MNS_Force_Node_Connected)
					eMNStatus = MNS_Force_Node_Listening;
	
				print(LL_INFO, "Disconnected from the Micronet.\n");
			}			
		}
		
		if (eMNStatus == MNS_NetworkChoice_N1 || eMNStatus == MNS_Force_Node_Listening)
		{
			int iPlace = FindMNDeviceInSyncList(mnIdMyDevice);
	
			if (iPlace != -1)
			{ //we're already on the list. Connected.
				if (eMNStatus == MNS_NetworkChoice_N1)
					eMNStatus = MNS_NetworkChoice_Node;
				else if (eMNStatus == MNS_Force_Node_Listening)
					eMNStatus = MNS_Force_Node_Connected;
	
				print(LL_INFO, "Connected to the Micronet as a node.\n");
			}
			else
			{
				if (!random(0, 6))
				{ //try to join the network. the random chance mimics MN devices' own anti-collision mechinism.
					print(LL_INFO, "Trying to connect to the Micronet as a node.\n");
					_uMNPacket pPacket;
					CreatePacket0x03(&pPacket);
					Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x03));				
				}
			}
		}
	}

	if (p->bMessage[8] != 0x03)
	{
		if (FindMNDeviceInSyncList(mnIdPacketDevice) == -1)
			print(LL_ERROR, "0x%.2X packet sent from device (%x:%x:%x:%x) not on the network.\n", p->bMessage[8], p->bMessage[4], p->bMessage[5], p->bMessage[6], p->bMessage[7]);
	}

//	if (iPacketTime < iPredictedWindow - 500 || iPacketTime > iPredictedWindow + 500)
//		print(LL_ERROR, "0x%.2x packet sent at wrong time. (predicted %d vs actual %d).\n", p->bMessage[8], iPredictedWindow, iPacketTime);

	if (eMNStatus == MNS_NetworkChoice_Scheduler || eMNStatus == MNS_Force_Scheduler)
	{//handle scheduler tasks.
		switch (p->bMessage[8])
		{
			case 0x03:
			{
				if (GetCRC(p->bMessage + 14, 2) != p->bMessage[16])
				{
					print(LL_ERROR, "Packet failed data CRC check. (CRC given %x expected %x)\n", p->bMessage[16], GetCRC(p->bMessage + 14, 2));
					goto end;
				}

				print(LL_INFO, "Adding node %2.2X %2.2X %2.2X %2.2X with a window size of %X.\n", p->bMessage[4], p->bMessage[5], p->bMessage[6], p->bMessage[7], p->bMessage[15]);
				sMNSync[iMNNodes].mnIdDevice = mnIdPacketDevice;
				sMNSync[iMNNodes].bWindowSize = p->bMessage[15];

				iMNNodes++;
			}
			break;
			case 0x04:
			{
				bool bFound = false;
				for (int i = 1; i < iMNNodes; i++)
				{
					if (bFound)
					{ //shift entry left
						sMNSync[i - 1].mnIdDevice = sMNSync[i].mnIdDevice;
					}

					if (sMNSync[i].mnIdDevice == mnIdPacketDevice)
						bFound = true;
				}

				if (bFound)
				{
					print(LL_INFO, "Removing node %2.2X %2.2X %2.2X %2.2X.\n", p->bMessage[4], p->bMessage[5], p->bMessage[6], p->bMessage[7]);
					iMNNodes--;
				}
			}
			break;
			case 0x05:
			{
				if (GetCRC(p->bMessage + 14, 1) != p->bMessage[15])
				{
					print(LL_ERROR, "Packet failed data CRC check. (CRC given %x expected %x)\n", p->bMessage[15], GetCRC(p->bMessage + 14, 1));
					goto end;
				}

				int iPlace = FindMNDeviceInSyncList(mnIdPacketDevice);
				if (iPlace != -1)
				{
					print(LL_INFO, "Adjusting node %2.2X %2.2X %2.2X %2.2X slot from %x to %x.\n", p->bMessage[4], p->bMessage[5], p->bMessage[6], p->bMessage[7], sMNSync[iPlace].bWindowSize, p->bMessage[14]);
					sMNSync[iPlace].bWindowSize = p->bMessage[14];
				}
			}
			break;
		}
	}
	else if (eMNStatus == MNS_NetworkChoice_Node || eMNStatus == MNS_Force_Node)
	{//handle client tasks.
		switch (p->bMessage[8])
		{
			case 0x01:
				tLAnnounce = p->tTime;
			break;
		}
	}

	if (eMNStatus > MNS_Connected)
	{//shared tasks
		switch (p->bMessage[8])
		{
			case 0x02:
			{ //This is 90% guess work!!!! I don't have Micronet stuff that sends data so I can't test it.
				int iPlace = 14;
				while (iPlace + 1 < p->bSize)
				{
					int iDataLength = p->bMessage[iPlace] + 2;

//						int a = int((unsigned char)(buffer[0]) << 24 |
//									(unsigned char)(buffer[1]) << 16 |
//									(unsigned char)(buffer[2]) << 8 |
//									(unsigned char)(buffer[3]));

					if (p->bMessage[iPlace + 1] == 0x01)
					{
						uint16_t iSpeed = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iSpeed = iSpeed;
						sDataArrayIn.tlSpeedUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x03)
					{
						sDataArrayIn.iWaterTemp = p->bMessage[iPlace + 3];
						sDataArrayIn.tlWaterTempUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x04)
					{
						uint16_t iDepth = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iDepth = iDepth;
						sDataArrayIn.tlDepthUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x05)
					{
						uint16_t iAppWindSpeed = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iAppWindSpeed = iAppWindSpeed;
						sDataArrayIn.tlAppWindSpeedUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x06)
					{
						uint16_t iAppWindDir = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iAppWindDir = iAppWindDir;
						sDataArrayIn.tlSpeedUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x07)
					{
						uint16_t iHeading = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iHeading = iHeading;
						sDataArrayIn.tlHeadingUpdate = p->tTime; 
					}
					else if (p->bMessage[iPlace + 1] == 0x1B)
					{
						uint16_t iVolts = ((unsigned char)(p->bMessage[iPlace + 3]) << 8 |
											(unsigned char)(p->bMessage[iPlace + 4]));
											
						sDataArrayIn.iVolts = iVolts;
						sDataArrayIn.tlVoltsUpdate = p->tTime; 
					}
					
					iPlace += iDataLength;
				}
			}
			break;
			case 0x06:
			{
				if (GetCRC(p->bMessage + 14, p->bSize - 15) != p->bMessage[p->bSize - 1])
				{
					print(LL_ERROR, "Packet failed data CRC check. (CRC given %x expected %x)\n", p->bMessage[p->bSize - 1], GetCRC(p->bMessage + 14, p->bSize - 15));
					goto end;
				}

				_uMNPacket pPacket;
				CreatePacket0x07(&pPacket);
				Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x07));

				tCommandBlock = tLAnnounce + 1500000; //block sending commands for the next round incase this command needs repeated.
				
				if (p->bMessage[14] == 0xFC && p->bMessage[15] == 0x7F)
				{ //request for node info
					sDataArrayOut.bSendNodeInfo = true;
				}
				else if (p->bMessage[14] == 0xE3 && p->bMessage[15] == 0x7F)
				{ //request for node info ended
					sDataArrayOut.bSendNodeInfo = false;
				}
				else if (p->bMessage[14] == 0xFA && p->bMessage[15] == 0x4F && p->bMessage[16] == 0x46 && p->bMessage[17] == 0x46)
				{ //remote network shutdown
					if (eMNStatus == MNS_NetworkChoice_Node) //tough call here. If we go back to network choice mode it'll just restart the network so we go to force node mode.
						eMNStatus = MNS_Force_Node; //MNS_NetworkChoice_N1;
					else if (eMNStatus == MNS_Force_Node_Connected)
						eMNStatus = MNS_Force_Node;
		
					print(LL_INFO, "Remote device commanded network shutdown.\n");
				}
			}
			break;
			case 0x07:
			{
			}
			break;
			case 0x0A:
			{
				_uMNPacket pPacket;
				CreatePacket0x0B(&pPacket);
				Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x0B));			
			}
			break;
		}
	}

end:
	if (bPrintReceives)
		PrintPacket(p, false, iPredictedWindow);

	if (p->bMessage)
		free(p->bMessage);
}

void IRAM_ATTR MicronetTimerCallback(TimerHandle_t xTimer)
{
	xTaskNotifyFromISR(mNet.thMicronetWorker, 0, eNoAction, NULL);
}

void Micronet::MicronetWorkerThread(void *m)
{
	for (;;)
	{
		ulTaskNotifyTake(1, portMAX_DELAY);

		mNet.MicronetWorker();
	}
}

void Micronet::MicronetWorker()
{
	uint64_t iCurrentTick = esp_timer_get_time();
	uint32_t iCurrentSecond = iCurrentTick / 1000000;
	uint32_t iCurrentUS = iCurrentTick - (uint64_t)(iCurrentSecond * 1000000); //0 - 999,999
	int iWorkerTickTime = iSyncPacketOffset + 500000; //we tick 500ms before/after sync packet.
	if (iWorkerTickTime >= 1000000)
		iWorkerTickTime -= 1000000;
		
	if (abs((int)(iCurrentUS - iWorkerTickTime)) < 50000)
	{
		switch (eMNStatus)
		{
			case MNS_TestMode1: //special test mode
			{
				print(LL_INFO, "Test mode 1: Tx inf. preamble for signal testing.\n");

			}
			break;
			case MNS_TestMode2: //special test mode
				print(LL_INFO, "Test mode 2.\n");
				eMNStatus = MNS_TestMode3;
			break;
			case MNS_TestMode3: //special test mode
				//print(LL_INFO, "Test mode 3.\n");
			break;
			case MNS_NetworkChoice: //start listening for a network.
				CCRadio.Listen();
				print(LL_INFO, "Network choice mode: Listening for a network.\n");
				eMNStatus = MNS_NetworkChoice_S1;
			break;
			case MNS_NetworkChoice_S1: //listen for another second.
				eMNStatus = MNS_NetworkChoice_S2;
			break;
			case MNS_NetworkChoice_S2:	//and a third.
				eMNStatus = MNS_NetworkChoice_M1;
			break;
			case MNS_NetworkChoice_M1: //no network so lets start one up.
			{
				print(LL_INFO, "Network choice mode: No network detected, announcing ourselves as the scheduler.\n");
				sMNSync[0].mnIdDevice = mnIdMyDevice;
				sMNSync[0].bWindowSize = 0;
				iMNNodes = 1;

				iSyncPacketOffset = 500000; //set the network sync packets to be sent at 0.5 seconds.
				uint64_t iCurrentSecond = esp_timer_get_time() / 1000000;
				uint64_t iNextTick = (iCurrentSecond + 1) * 1000000 + iSyncPacketOffset + (MN_BYTEXMITTIME * (MN_ANNOUNCEMENTLENGTH + 2));

				_uMNPacket pPacket;
				CreatePacket0x01(&pPacket);
				Send(pPacket, iNextTick, MN_ANNOUNCEMENTLENGTH);

				eMNStatus = MNS_NetworkChoice_M2;
			}
			break;
			case MNS_NetworkChoice_M2: //do nothing while waiting for our multi-second announcement packet to send.
			break;
			case MNS_NetworkChoice_Scheduler:
			{
				uint64_t iNextSyncWindow = (iCurrentSecond * 1000000) + iSyncPacketOffset;

				_uMNPacket pPacket;
				CreatePacket0x01(&pPacket);
				Send(pPacket, iNextSyncWindow);

				if (iCurrentTick - tNextPing < 1000000)
				{ //network ping this sync.
					tNextPing += MN_PINGINTERVALS * 1000000;

					int iCommandWindow = 7400;
					for (int i = 0; i < iMNNodes; i++)
						if (sMNSync[i].bWindowSize != 0)
							iCommandWindow += 5133 + (int(sMNSync[i].bWindowSize / 2.346) + 1) * 244;

//fixme!!					Send(0x0A, NULL, 0, iNextSyncWindow + iCommandWindow + 500);
				}
			}
			break;
			case MNS_Force_Node: //start listening for a network.
			{
				CCRadio.Listen();
				print(LL_INFO, "Network forced node mode: Listening for a network.\n");

				eMNStatus = MNS_Force_Node_Listening;
			}
			break;
		}

		if (eMNStatus > MNS_Connected)
		{
			if (iCurrentTick - tLAnnounce > 5 * 1000000) //no packets for 5 seconds means we're disconnected.
			{
				if (eMNStatus == MNS_NetworkChoice_Node)
					eMNStatus = MNS_NetworkChoice_N1;
				else if (eMNStatus == MNS_Force_Node_Connected)
					eMNStatus = MNS_Force_Node_Listening;
	
				print(LL_INFO, "Disconnected from the Micronet.\n");			
			}

			int iPlace = FindMNDeviceInSyncList(mnIdMyDevice);
			
			_uMNPacket pPacket;
			CreatePacket0x02(&pPacket);
			if (pPacket.sHeader.bLength > 14)
			{
				if (sMNSync[iPlace].bWindowSize < pPacket.sHeader.bLength)
				{ //we have to increase our window before sending this data.
					print(LL_INFO, "Adjusting our window size from %x to %x.\n", sMNSync[iPlace].bWindowSize, pPacket.sHeader.bLength);
										
					int iNewSize = pPacket.sHeader.bLength;
					CreatePacket0x05(&pPacket, iNewSize);
					Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x05));
				}
				else
					Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x02));
			}
			else if (sMNSync[iPlace].bWindowSize > 0)
			{ //we have a window but no data to send so we mute our window.
				print(LL_INFO, "Adjusting our window size from %x to %x.\n", sMNSync[iPlace].bWindowSize, 0);
									
				CreatePacket0x05(&pPacket, 0);
				Send(pPacket, GetNextPacketWindow(mnIdMyDevice, 0x05));				
			}

		}

		for (int i = 0; i < 10; i++)
			if (sTimerHandlers[i].cbCallback)
			{
				try
				{
					sTimerHandlers[i].cbCallback(sDataArrayIn, sTimerHandlers[i].vArg);
				}
				catch(...)
				{
					print(LL_ERROR, "Execption while calling timer handlers.\n");	
				}
			}
	}
}

void Micronet::SentPacketCallback(void *m, _sPacket *p, bool bSent)
{
	Micronet *mNet = (Micronet *)m;
	mNet->CCRadio.Listen();

	if (bSent)
	{
		if (p->bMessage[8] == 0x01)
			mNet->tLAnnounce = p->tTime;

		if (mNet->eMNStatus == MNS_NetworkChoice_M2 && p->bMessage[8] == 0x01)
		{
			print(LL_INFO, "Network choice mode: Now operating as the network scheduler.\n");

			mNet->tNextPing = esp_timer_get_time() + (MN_PINGINTERVALS * 1000000);
			mNet->eMNStatus = MNS_NetworkChoice_Scheduler;
		}

		if (mNet->bPrintSends)
			mNet->PrintPacket(p, true, mNet->GetPacketWindow(mNet->mnIdMyDevice, p->bMessage[8]));
	}
}

void Micronet::AddTimerHandler(void (*cbCallback)(_sDataArrayIn sDataIn, void *v2), void *vArg)
{
	for (int i = 0; i < 10; i++)
		if (!sTimerHandlers[i].cbCallback)
		{
			sTimerHandlers[i].cbCallback = cbCallback;
			sTimerHandlers[i].vArg = vArg;

			return;
		}
}

void Micronet::RemoveTimerHandler(void (*cbCallback)(_sDataArrayIn sDataIn, void *v2))
{
	for (int i = 0; i < 10; i++)
		if (sTimerHandlers[i].cbCallback == cbCallback)
			sTimerHandlers[i].cbCallback = NULL;
}

void Micronet::Send(_uMNPacket pPacket, uint64_t tSendTime, int iPreambleLength)
{
	if (pPacket.sHeader.bLength + 2 > MN_MAXPACKETSIZE)
	{
		print(LL_ERROR, "Micronet::Send(): iLength too long (%d).\n", pPacket.sHeader.bLength + 2);
		return;
	}

	if (tCommandBlock > tSendTime && (pPacket.sHeader.bPacketType == 0x03 || pPacket.sHeader.bPacketType == 0x04 || pPacket.sHeader.bPacketType == 0x05 || pPacket.sHeader.bPacketType == 0x06 || pPacket.sHeader.bPacketType == 0x08 || pPacket.sHeader.bPacketType == 0x0A))
	{
		print(LL_INFO, "Command window expected to be occupied this round, dropping our own command.\n");
		return;	
	}

	//fixme. CC1101's send latancy is 8 bits we should account for it.
	tSendTime -= (MN_BYTEXMITTIME * (iPreambleLength + 2)); //account for our preamble, syncword and starting zero packet.

	int iDelay = tSendTime - esp_timer_get_time();
	if (iDelay < -100)
	{
		print(LL_ERROR, "Micronet::Send(): Trying to send a packet to the past. (iDelay %d tSendTime %d).\n", iDelay, (uint32_t)tSendTime);
		return;
	}

	print(LL_DEBUG, "Scheduling to send 0x%x packet at %d.\n", pPacket.sHeader.bPacketType, (uint32_t)tSendTime);
	CCRadio.Send(pPacket.bRaw, pPacket.sHeader.bLength + 2, tSendTime, iPreambleLength, &SentPacketCallback, (void *)this);
}

int Micronet::FindMNDeviceInSyncList(_cMNId mnId)
{
	int iPlace = -1;
	for (int i = 0; i < iMNNodes; i++)
		if (sMNSync[i].mnIdDevice == mnId)
		{
			iPlace = i;
			break;
		}

	return iPlace;
}

byte Micronet::GetCRC(byte *b, int len)
{
	byte ret = 0;
	for (int i = 0; i < len; i++)
		ret += b[i];

	return ret;
}

int Micronet::GetPacketWindow(_cMNId mnDevice, byte bPacketType, bool bFuture)
{
	int iPredictedWindow = 0;

	if (bPacketType == 0x01)
	{
		return iPredictedWindow;
	}
	else if (bPacketType == 0x02)
	{ //first round packets.
		iPredictedWindow += 200; 
		bool bFound = false;
		for (int i = 0; i < iMNNodes; i++)
		{
			if (sMNSync[i].mnIdDevice == mnDevice)
			{
				bFound = true;
				break;
			}

			if (sMNSync[i].bWindowSize != 0)
				iPredictedWindow += 5133 + (int(sMNSync[i].bWindowSize / 2.346) + 1) * 244;
		}

		if (!bFound)
			return -1;
	}
	else if (bPacketType == 0x03 || bPacketType == 0x04 || bPacketType == 0x05 || bPacketType == 0x06 || bPacketType == 0x08 || bPacketType == 0x0A)
	{ //packets that share the command slot. 
		iPredictedWindow += 7250;
		for (int i = 0; i < iMNNodes; i++)
			if (sMNSync[i].bWindowSize != 0)
				iPredictedWindow += 5133 + (int(sMNSync[i].bWindowSize / 2.346) + 1) * 244;
	}
	else if (bPacketType == 0x07 || bPacketType == 0x0B)
	{ //second (reversed) round packets.
		iPredictedWindow += 7250 + 1700;
		for (int i = 0; i < iMNNodes; i++)
			if (sMNSync[i].bWindowSize != 0)
				iPredictedWindow += 5133 + (int(sMNSync[i].bWindowSize / 2.346) + 1) * 244;

		for (int i = iMNNodes - 1; i >= 0; i--)
		{
			iPredictedWindow += 5400;
			
			if (sMNSync[i].mnIdDevice == mnDevice)
				break;
		}
	}

	uint64_t iCurrentTick = esp_timer_get_time();
	while (bFuture && (tLAnnounce + iPredictedWindow) < iCurrentTick)
		iPredictedWindow += 1000000;

//	if (bFuture)
//		print(LL_NONE, "%d - %d = %d\n", (uint32_t)esp_timer_get_time(), (uint32_t)iNextTick, (uint32_t)(esp_timer_get_time() - iNextTick));

	return iPredictedWindow;
}

void Micronet::PrintPacket(_sPacket *p, bool bSR, int iPrediction)
{
	uint64_t time = p->tTime - mNet.tLAnnounce;

	if (p->bMessage[8] == 0x01)
		iPrediction = 0;

	print(LL_DEBUG, "%s %03.3d bytes. %+6.6d μs: (p%.6d/d%+.5d) | ", (bSR ? "TX" : "RX"), p->bSize, (uint32_t)time, iPrediction, time - iPrediction);
	for (int i = 0; i < p->bSize; i++)
	{
		if (i == 3 ||
			i == 7 ||
			i == 8 ||
			i == 11 ||
			i == 13)
			printnts(LL_DEBUG, "%.2X | ", p->bMessage[i]);
		else
			printnts(LL_DEBUG, "%.2X ", p->bMessage[i]);
	}
	printnts(LL_DEBUG, "\n");
}

void Micronet::CreatePacket0x01(_uMNPacket *p)
{
	if (!p)
		return;
	
	int iLength = 3 + (5 * iMNNodes);
	
	sMNSync[0].bWindowSize = iLength;

	for (int i = 0; i < iMNNodes; i++)
	{
		p->sHeader.bData[i * 5 + 0] = sMNSync[i].mnIdDevice.bId[0];
		p->sHeader.bData[i * 5 + 1] = sMNSync[i].mnIdDevice.bId[1];
		p->sHeader.bData[i * 5 + 2] = sMNSync[i].mnIdDevice.bId[2];
		p->sHeader.bData[i * 5 + 3] = sMNSync[i].mnIdDevice.bId[3];
		p->sHeader.bData[i * 5 + 4] = sMNSync[i].bWindowSize;
	}

	p->sHeader.bData[iMNNodes * 5 + 0] = 0;
	p->sHeader.bData[iMNNodes * 5 + 1] = 0;
	p->sHeader.bData[iMNNodes * 5 + 2] = GetCRC(p->sHeader.bData, iLength - 1);

	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x01;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::CreatePacket0x02(_uMNPacket *p)
{
	if (!p)
		return;

	uint64_t tCurrentTick = esp_timer_get_time();
	int iLength = 0;

/*bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;*/
  
	if (tCurrentTick - sDataArrayOut.tlSpeedUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x01;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iSpeed >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iSpeed & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlTripLogUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x0A;
		p->sHeader.bData[iLength + 1] = 0x02;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iTrip >> 24) & 0xFF; 
		p->sHeader.bData[iLength + 4] = (sDataArrayOut.iTrip >> 16) & 0xFF; 
		p->sHeader.bData[iLength + 5] = (sDataArrayOut.iTrip >> 8) & 0xFF; 
		p->sHeader.bData[iLength + 6] = (sDataArrayOut.iTrip >> 0) & 0xFF; 
		p->sHeader.bData[iLength + 7] = (sDataArrayOut.iLog >> 24) & 0xFF;
		p->sHeader.bData[iLength + 8] = (sDataArrayOut.iLog >> 16) & 0xFF; 
		p->sHeader.bData[iLength + 9] = (sDataArrayOut.iLog >> 8) & 0xFF; 
		p->sHeader.bData[iLength + 10] = (sDataArrayOut.iLog >> 0) & 0xFF; 
		p->sHeader.bData[iLength + 11] = GetCRC(p->sHeader.bData + iLength, 11);

		iLength += 12;
	}

	if (tCurrentTick - sDataArrayOut.tlWaterTempUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x03;
		p->sHeader.bData[iLength + 1] = 0x03;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iWaterTemp >> 0) & 0xFF;
		p->sHeader.bData[iLength + 4] = GetCRC(p->sHeader.bData + iLength, 4);

		iLength += 5;
	}

	if (tCurrentTick - sDataArrayOut.tlDepthUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x04;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iDepth >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iDepth & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}
	
	if (tCurrentTick - sDataArrayOut.tlAppWindSpeedUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x05;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iAppWindSpeed >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iAppWindSpeed & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlAppWindDirUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x06;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iAppWindDir >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iAppWindDir & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}
	
	if (tCurrentTick - sDataArrayOut.tlHeadingUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x07;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iHeading >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iHeading & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlSOGCOGUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x06;
		p->sHeader.bData[iLength + 1] = 0x08;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iSOG >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iSOG & 0xFF;
		p->sHeader.bData[iLength + 5] = (sDataArrayOut.iCOG >> 8) & 0xFF;
		p->sHeader.bData[iLength + 6] = sDataArrayOut.iCOG & 0xFF;
		p->sHeader.bData[iLength + 7] = GetCRC(p->sHeader.bData + iLength, 7);

		iLength += 8;
	}
	
	if (tCurrentTick - sDataArrayOut.tlLatLongUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x09;
		p->sHeader.bData[iLength + 1] = 0x09;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = sDataArrayOut.iLatDegree & 0xFF;
		p->sHeader.bData[iLength + 4] = (sDataArrayOut.iLatMinute >> 8) & 0xFF;
		p->sHeader.bData[iLength + 5] = sDataArrayOut.iLatMinute & 0xFF;
		p->sHeader.bData[iLength + 6] = sDataArrayOut.iLongDegree & 0xFF;
		p->sHeader.bData[iLength + 7] = (sDataArrayOut.iLongMinute >> 8) & 0xFF;
		p->sHeader.bData[iLength + 8] = sDataArrayOut.iLongMinute & 0xFF;
		p->sHeader.bData[iLength + 9] = sDataArrayOut.iNESW & 0xFF;
		p->sHeader.bData[iLength + 10] = GetCRC(p->sHeader.bData + iLength, 10);

		iLength += 11;
	}
	
	if (tCurrentTick - sDataArrayOut.tlBTWUpdate < 1000000)
	{
		char cBuff[4];
		int iLen = strlen(sDataArrayOut.cWPName);

		if (iLen < 5)
		{
			memcpy(cBuff, sDataArrayOut.cWPName, 4);
		}
		else
		{
			int iOffset = sDataArrayOut.iWPNameScrollPos;
			
			for (int i = 0; i < 4; i++)
			{
				if (sDataArrayOut.cWPName[iOffset + i] == '\0')
				{
					cBuff[i] = ' ';
					iOffset = -i - 1;
				}
				else
					cBuff[i] = sDataArrayOut.cWPName[iOffset + i];
	
			}
	
			sDataArrayOut.iWPNameScrollPos++;
			if (sDataArrayOut.iWPNameScrollPos > 14 || sDataArrayOut.iWPNameScrollPos > iLen)
				sDataArrayOut.iWPNameScrollPos = 0;
		}				

		p->sHeader.bData[iLength + 0] = 0x0A;
		p->sHeader.bData[iLength + 1] = 0x0A;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iBTW >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iBTW & 0xFF;
		p->sHeader.bData[iLength + 5] = 0; //unk
		p->sHeader.bData[iLength + 6] = 0; //unk
		p->sHeader.bData[iLength + 7] = cBuff[0];
		p->sHeader.bData[iLength + 8] = cBuff[1];
		p->sHeader.bData[iLength + 9] = cBuff[2];
		p->sHeader.bData[iLength + 10] = cBuff[3];

		p->sHeader.bData[iLength + 11] = GetCRC(p->sHeader.bData + iLength, 11);

		iLength += 12;
	}

	if (tCurrentTick - sDataArrayOut.tlXTEUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x0B;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iXTE >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iXTE & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlTimeUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x0C;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = sDataArrayOut.iHour & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iMinute & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlDateUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x05;
		p->sHeader.bData[iLength + 1] = 0x0D;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = sDataArrayOut.iDay & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iMonth & 0xFF;
		p->sHeader.bData[iLength + 5] = sDataArrayOut.iYear & 0xFF;
		p->sHeader.bData[iLength + 6] = GetCRC(p->sHeader.bData + iLength, 6);

		iLength += 7;
	}

	if (tCurrentTick - sDataArrayOut.tlVMG_WPUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x12;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iVMG_WP >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iVMG_WP & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}

	if (tCurrentTick - sDataArrayOut.tlVoltsUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x04;
		p->sHeader.bData[iLength + 1] = 0x1B;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iVolts >> 8) & 0xFF;
		p->sHeader.bData[iLength + 4] = sDataArrayOut.iVolts & 0xFF;
		p->sHeader.bData[iLength + 5] = GetCRC(p->sHeader.bData + iLength, 5);

		iLength += 6;
	}


	if (tCurrentTick - sDataArrayOut.tlDTWUpdate < 1000000)
	{
		p->sHeader.bData[iLength + 0] = 0x06;
		p->sHeader.bData[iLength + 1] = 0x1F;
		p->sHeader.bData[iLength + 2] = 0x09;
		p->sHeader.bData[iLength + 3] = (sDataArrayOut.iDTW >> 24) & 0xFF;
		p->sHeader.bData[iLength + 4] = (sDataArrayOut.iDTW >> 16) & 0xFF;
		p->sHeader.bData[iLength + 5] = (sDataArrayOut.iDTW >> 8) & 0xFF;
		p->sHeader.bData[iLength + 6] = sDataArrayOut.iDTW & 0xFF;
		p->sHeader.bData[iLength + 7] = GetCRC(p->sHeader.bData + iLength, 7);

		iLength += 8;
	}

	if (sDataArrayOut.bSendNodeInfo)
	{
		p->sHeader.bData[iLength + 0] = 0x06;
		p->sHeader.bData[iLength + 1] = 0x10;
		p->sHeader.bData[iLength + 2] = sDataArrayOut.bNodeInfoType; //type, 0=display (guess), 1=hull, 2=wind, 3=nmea, 4=mast , 5=mob, 6=sdpod, 7=type7, 8=type8,
		p->sHeader.bData[iLength + 3] = sDataArrayOut.bNodeInfoVMinor; //minor version
		p->sHeader.bData[iLength + 4] = sDataArrayOut.bNodeInfoVMajor; //major version
		p->sHeader.bData[iLength + 5] = 0x33; //battery/sun. 0x01 = 1 bar battery, 0x02 = 2 bar battery, 0x03 = 3 bar battery, 0x10 = 1 bar sun, 0x20 = 2 bar sun, 0x30 = 3 bar sun, 
		p->sHeader.bData[iLength + 6] = 0x08;
		p->sHeader.bData[iLength + 7] = GetCRC(p->sHeader.bData + iLength, 7); //0x64;

		iLength += 8;
	}

/*		p->sHeader.bData[iLength + 0] = 0x06;
		p->sHeader.bData[iLength + 1] = 0xFD;
		p->sHeader.bData[iLength + 2] = 0x00; //unk
		p->sHeader.bData[iLength + 3] = 0x01; //minutes
		p->sHeader.bData[iLength + 4] = 0x00; //seconds
		p->sHeader.bData[iLength + 5] = 0x00;
		p->sHeader.bData[iLength + 6] = 0x00; //0x00 = stop, 0x01 = run
		p->sHeader.bData[iLength + 7] = GetCRC(p->sHeader.bData + iLength, 7); //0x64;

		iLength += 8;
*/


	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x02;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::CreatePacket0x03(_uMNPacket *p)
{
	if (!p)
		return;
		
	int iLength = 3;
	
	p->sHeader.bData[0] = 0;
	p->sHeader.bData[1] = 0;
	p->sHeader.bData[2] = GetCRC(p->sHeader.bData, iLength - 1);

	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x03;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::CreatePacket0x05(_uMNPacket *p, int iNewWindowSize)
{
	if (!p)
		return;
		
	int iLength = 2;
	p->sHeader.bData[0] = iNewWindowSize;
	p->sHeader.bData[1] = GetCRC(p->sHeader.bData, iLength - 1);
	
	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x05;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::CreatePacket0x07(_uMNPacket *p)
{
	if (!p)
		return;
		
	int iLength = 0;

	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x07;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::CreatePacket0x0B(_uMNPacket *p)
{
	if (!p)
		return;
		
	int iLength = 2;
	
	p->sHeader.bData[0] = 0;
	p->sHeader.bData[0] = GetCRC(p->sHeader.bData, iLength - 1);
	
	p->sHeader.mnIdNetwork = mnIdMyNetwork;
	p->sHeader.mnIdDevice = mnIdMyDevice;
	p->sHeader.bPacketType = 0x0B;
	p->sHeader.bUnk1 = 0x09;
	p->sHeader.bSignalQuality = 0; //fixme. put rssi here
	p->sHeader.bChecksum = GetCRC(p->bRaw, 11);
	p->sHeader.bLengthChecksum = p->sHeader.bLength = iLength + 12;	
}

void Micronet::SetSpeed(float fSpeed)
{
	sDataArrayOut.tlSpeedUpdate = esp_timer_get_time();
	sDataArrayOut.iSpeed = (uint16_t)(fSpeed * 100.0f);
}

void Micronet::SetTripLog(float fTrip, float fLog)
{
	sDataArrayOut.tlTripLogUpdate = esp_timer_get_time();
	sDataArrayOut.iTrip = (uint32_t)(fTrip * 100.0f);
	sDataArrayOut.iLog = (uint32_t)(fLog * 10.0f);
}

void Micronet::SetWaterTemperature(float fTemp)
{
	sDataArrayOut.tlWaterTempUpdate = esp_timer_get_time();
	sDataArrayOut.iWaterTemp = (int8_t)(fTemp * 2.0f);
}

void Micronet::SetDepth(float fDepth)
{
	sDataArrayOut.tlDepthUpdate = esp_timer_get_time();

	sDataArrayOut.iDepth = (uint16_t)(fDepth * 10.0f);
}

void Micronet::SetApparentWindSpeed(float fWindSpeed)
{
	sDataArrayOut.tlAppWindSpeedUpdate = esp_timer_get_time();
	sDataArrayOut.iAppWindSpeed = (uint16_t)(fWindSpeed * 10.0f);
}

void Micronet::SetApparentWindDirection(float fWindDir)
{
	sDataArrayOut.tlAppWindDirUpdate = esp_timer_get_time();
	sDataArrayOut.iAppWindDir = (int16_t)fWindDir;
}

void Micronet::SetHeading(float fHeading)
{
	sDataArrayOut.tlHeadingUpdate = esp_timer_get_time();
	sDataArrayOut.iHeading = (int16_t)fHeading;
}

void Micronet::SetSOGCOG(float fSOG, float fCOG)
{
	sDataArrayOut.tlSOGCOGUpdate = esp_timer_get_time();
	sDataArrayOut.iSOG = (uint16_t)(fSOG * 10.0f);
	sDataArrayOut.iCOG = (uint16_t)fCOG;
}

void Micronet::SetLatLong(float fLat, float fLong)
{
	sDataArrayOut.tlLatLongUpdate = esp_timer_get_time();
	
	sDataArrayOut.iNESW = 0;
	if (fLat > 0)
		sDataArrayOut.iNESW = 0x01;
	if (fLong > 0) 
		sDataArrayOut.iNESW += 0x02;

	fLat = abs(fLat);
	fLong = abs(fLong);
	
	sDataArrayOut.iLatDegree = (uint8_t)fLat;
	sDataArrayOut.iLatMinute = (uint16_t)((fLat - sDataArrayOut.iLatDegree) * 60.0f * 1000.0f);
	sDataArrayOut.iLongDegree = (uint8_t)fLong;
	sDataArrayOut.iLongMinute = (uint16_t)((fLong - sDataArrayOut.iLongDegree) * 60.0f * 1000.0f);
}

void Micronet::SetBTW(float fBTW)
{
	sDataArrayOut.tlBTWUpdate = esp_timer_get_time();

	sDataArrayOut.iBTW = (uint16_t) fBTW;
}

void Micronet::SetWPName(char *cWPName)
{
	sDataArrayOut.tlBTWUpdate = esp_timer_get_time();

	for (int i = 0; i < 16; i++)
		if (cWPName[i] > 31 && cWPName[i] < 91)
			sDataArrayOut.cWPName[i] = cWPName[i];
		else
			cWPName[i] = 0;
}

void Micronet::SetXTE(float fXTE)
{
	sDataArrayOut.tlXTEUpdate = esp_timer_get_time();

	sDataArrayOut.iXTE = (int16_t)(fXTE * 100.0f);
}

void Micronet::SetTime(uint8_t iHour, uint8_t iMinute)
{
	sDataArrayOut.tlTimeUpdate = esp_timer_get_time();

	sDataArrayOut.iHour = iHour;
	sDataArrayOut.iMinute = iMinute;
}

void Micronet::SetDate(uint8_t iDay, uint8_t iMonth, uint8_t iYear)
{
	sDataArrayOut.tlDateUpdate = esp_timer_get_time();

	sDataArrayOut.iDay = iDay;
	sDataArrayOut.iMonth = iMonth;
	sDataArrayOut.iYear = iYear;
}

void Micronet::SetVMG_WP(float fSpeed)
{
	sDataArrayOut.tlVMG_WPUpdate = esp_timer_get_time();
	
	sDataArrayOut.iVMG_WP = (uint16_t)(fSpeed * 100.0f);
}

void Micronet::SetVolts(float fVolts)
{
	sDataArrayOut.tlVoltsUpdate = esp_timer_get_time();

	sDataArrayOut.iVolts = (uint16_t)(fVolts * 10.0f);
}

void Micronet::SetDTW(float fDistance)
{
	sDataArrayOut.tlDTWUpdate = esp_timer_get_time();

	sDataArrayOut.iDTW = (uint32_t)(fDistance * 100.0f);
}
