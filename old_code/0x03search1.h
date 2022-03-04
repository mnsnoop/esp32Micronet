#ifndef _SEARCH01_H
#define _SEARCH01_H

#include "micronet.h"

class Search01
{
private:
	Micronet *mNet = NULL;
	enum
	{
		SS_Cleanup,
		SS_Setup,
		SS_Window, //quick forward search
		SS_WindowStart_Test, //test to see if the current window is still working. If not slowly step until it is.
		SS_WindowStart_Search, //slow backward search
		SS_WindowEnd //slow forward search
		
	} eSS = SS_Cleanup;


	int iTWindows[32] = {171, 105, 96, 43, 0, 83, 0, 67, 58, 70, 132, 0, 48, 73, 0, 117, 113, 62, 0, 135};
	int iTWindowCount = 21;
	

	bool bLastActionAdding = false;
	int iDelay = 3000,
		iWorkingDelayCurrentNode = 0,
		iLWorkingDelay = 0,
		iLWindow = 0,
		iMissed = 0;

public:

	bool bPaused = 1,
		 bL = 0;

	void Setup(Micronet *m);
	void static IncomingPacketCB(void *v, void *v2);
};

#endif
