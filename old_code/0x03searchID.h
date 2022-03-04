#ifndef _SEARCHID_H
#define _SEARCHID_H

#include "micronet.h"

#define MAXSAMPLES 10

class SearchID
{
private:
	void TimerCB(void *v);
	void IncomingPacketCB(void *v);
	
	Micronet *mNet = NULL;

	uint64_t tSamples[MAXSAMPLES];
	int iSample = 0,
		iCurrentWindowSize = 0;
	
public:
	int iMNNodes;

	bool bPaused = 1,
		 bL = 0,
		 bAnnounce = false;

	void Setup(Micronet *m);
	void static TimerCB(void *v, void *v2)
	{
		SearchID *s = (SearchID *)v;
		s->TimerCB(v2);
	}

	void static IncomingPacketCB(void *v, void *v2)
	{
		SearchID *s = (SearchID *)v;
		s->IncomingPacketCB(v2);
	}
};

#endif
