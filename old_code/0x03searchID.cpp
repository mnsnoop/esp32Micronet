#include "0x03searchID.h"

void SearchID::Setup(Micronet *m)
{
	mNet = m;

	m->AddTimerCallback(&TimerCB, (void *)this);
	m->AddIncomingPacketCallback(&IncomingPacketCB, (void *)this);
}

void SearchID::TimerCB(void *v)
{
	_sPacket *p = (_sPacket *)v;

	byte bPacket[200];

	iMNNodes = 3;

	bPacket[0] = mNet->bDeviceId[0];
	bPacket[1] = mNet->bDeviceId[1];
	bPacket[2] = mNet->bDeviceId[2];
	bPacket[3] = mNet->bDeviceId[3];
	bPacket[4] = 3 + (5 * iMNNodes);
	
	for (int i = 1; i < iMNNodes; i++)
	{
		bPacket[i * 5 + 0] = 1;
		bPacket[i * 5 + 1] = 1;
		bPacket[i * 5 + 2] = 1;
		bPacket[i * 5 + 3] = i + 1;
		bPacket[i * 5 + 4] = 6;
	}

	bPacket[iMNNodes * 5 - 1] = iCurrentWindowSize;
	
	bPacket[iMNNodes * 5 + 0] = 0;
	bPacket[iMNNodes * 5 + 1] = 0;
	bPacket[iMNNodes * 5 + 2] = GetCRC(bPacket, iMNNodes * 5 + 2);


	uint64_t tCurrentTick = esp_timer_get_time();
	uint64_t iSeconds = tCurrentTick / 1000000;
	uint64_t iNextTick = (iSeconds + 1) * 1000000 + 50000;

	mNet->Send(0x1, bPacket, iMNNodes * 5 + 3, iNextTick);
}

void SearchID::IncomingPacketCB(void *v)
{
	_sPacket *p = (_sPacket *)v;

	if (p->bMessage[8] == 0x03)
	{
		tSamples[iSample] = p->tTime - mNet->tLAnnounce;
		iSample++;

		if (iSample >= MAXSAMPLES)
		{
			int tWindow = 0;
			for (int i = 0; i < MAXSAMPLES; i++)
				tWindow += tSamples[i];
			tWindow /= MAXSAMPLES;
			
			print("\t%d\t%d\t%d\t%d\t%d\n", iMNNodes, 3 + (5 * iMNNodes), iCurrentWindowSize, 3 + (5 * iMNNodes) + iCurrentWindowSize + 6, tWindow - 1771);
			iSample = 0;
			iCurrentWindowSize++;
		}
	}
}
