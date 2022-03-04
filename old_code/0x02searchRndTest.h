#ifndef _SEARCHRNDTEST_H
#define _SEARCHRNDTEST_H

#include "micronet.h"

#define MAXSAMPLES 6

class SearchRndTest
{
private:
	void TimerCB(void *v);
	void IncomingPacketCB(void *v);
	
	Micronet *mNet = NULL;

	uint64_t tSamples[MAXSAMPLES];
	int iSample = 0,
		iMNNodes = 0,
		iPredictedWindow = 0,
		iTWS = 0;
		
	_sMNPSync sSync[32];
		
public:
	bool bPaused = 1,
		 bL = 0,
		 bAnnounce = false;

	void RandomizeSync();
	void PrintSync();

	void Setup(Micronet *m);
	void static TimerCB(void *v, void *v2)
	{
		SearchRndTest *s = (SearchRndTest *)v;
		s->TimerCB(v2);
	}

	void static IncomingPacketCB(void *v, void *v2)
	{
		SearchRndTest *s = (SearchRndTest *)v;
		s->IncomingPacketCB(v2);
	}
};

#endif
