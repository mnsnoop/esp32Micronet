#include "0x03searchRndTest.h"

void SearchRndTest::RandomizeSync()
{
	iMNNodes = 1 + random(1, 29);
//	iMNNodes = 3;

	sSync[0].bDeviceId[0] = mNet->bDeviceId[0];
	sSync[0].bDeviceId[1] = mNet->bDeviceId[1];
	sSync[0].bDeviceId[2] = mNet->bDeviceId[2];
	sSync[0].bDeviceId[3] = mNet->bDeviceId[3];
	sSync[0].bWindowSize = 3 + (5 * iMNNodes);

	iTWS = 0;
	for (int i = 1; i < iMNNodes; i++)
	{
		sSync[i].bDeviceId[0] = mNet->bDeviceId[0];
		sSync[i].bDeviceId[1] = mNet->bDeviceId[1];
		sSync[i].bDeviceId[2] = mNet->bDeviceId[2];
		sSync[i].bDeviceId[3] = i + 1;

		int iW = random(0, 245);
		iW -= 80;
		if (iW <= 0) 
			iW = 0;

		sSync[i].bWindowSize = iW;
		iTWS += sSync[i].bWindowSize;
	}

	if (iTWS > 2000)
		RandomizeSync();
}

void SearchRndTest::Setup(Micronet *m)
{
	mNet = m;

	m->AddTimerCallback(&TimerCB, (void *)this);
	m->AddIncomingPacketCallback(&IncomingPacketCB, (void *)this);

	RandomizeSync();

	print("Nodes,Window Sizes,TWS,Window Actual,Window Predicted,Diff\n");
}

void SearchRndTest::TimerCB(void *v)
{
	byte bPacket[200];
	
	for (int i = 0; i < iMNNodes; i++)
	{
		bPacket[i * 5 + 0] = sSync[i].bDeviceId[0];
		bPacket[i * 5 + 1] = sSync[i].bDeviceId[1];
		bPacket[i * 5 + 2] = sSync[i].bDeviceId[2];
		bPacket[i * 5 + 3] = sSync[i].bDeviceId[3];
		bPacket[i * 5 + 4] = sSync[i].bWindowSize;
	}

	bPacket[iMNNodes * 5 + 0] = 0;
	bPacket[iMNNodes * 5 + 1] = 0;
	bPacket[iMNNodes * 5 + 2] = GetCRC(bPacket, iMNNodes * 5 + 2);

	uint64_t tCurrentTick = esp_timer_get_time();
	uint64_t iSeconds = tCurrentTick / 1000000;
	uint64_t iNextTick = (iSeconds + 1) * 1000000 + 50000;

	mNet->Send(0x1, bPacket, iMNNodes * 5 + 3, iNextTick);
}

void SearchRndTest::IncomingPacketCB(void *v)
{
	_sPacket *p = (_sPacket *)v;

	if (p->bMessage[8] == 0x03)
	{
		tSamples[iSample] = p->tTime - mNet->tLAnnounce;
		iSample++;

		if (iSample >= MAXSAMPLES)
		{
			int tWindow = 0;
			for (int i = 1; i < MAXSAMPLES; i++) //skip first sample.
				tWindow += tSamples[i];
			tWindow /= MAXSAMPLES - 1;
			iSample = 0;
		
			iPredictedWindow = 5400 + 2000; //5400 to window start, +2000 to when MN devices normally send.
			print("%d,", iMNNodes);
			for (int i = 0; i < iMNNodes; i++)
			{
				if (sSync[i].bWindowSize == 0)
					iPredictedWindow += 0;
				else
					iPredictedWindow += 5133 + (int(sSync[i].bWindowSize / 2.346) + 1) * 244;

				printnts(" %d", sSync[i].bWindowSize);
			}
			printnts(",%d,%d,%d,%d\n", iTWS, iPredictedWindow, tWindow, iPredictedWindow - tWindow);
			
			RandomizeSync();
		}
	}
}
