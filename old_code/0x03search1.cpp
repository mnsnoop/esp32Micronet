#include "0x03search1.h"

void Search01::Setup(Micronet *m)
{
	mNet = m;

	m->AddIncomingPacketCallback(&IncomingPacketCB, (void *)this);
}

void Search01::IncomingPacketCB(void *v, void *v2)
{
	Search01 *s01 = (Search01 *)v;
	_sPacket *p = (_sPacket *)v2;
	
	if (p->bMessage[8] == 0x01)
	{
		if (GetCRC(p->bMessage + 14, p->bSize - 15) != p->bMessage[p->bSize - 1])
		{
//				print("Micronet::IncomingPacketHandler(): CRC error in sync packet.\n");
			return;
		}

		_sMNPSync *sMNPSync;
		int iOurSlot = -1,
			iNodes = 0,
			iFWS = p->bMessage[18],
			iTWS = 0,
			iPredictedWindow = 7500;
			
		for (int i = 14; i < p->bSize - 5; i += 5)
		{
			sMNPSync = (_sMNPSync *)(p->bMessage + i);
			iNodes++;
			iTWS += sMNPSync->bWindowSize;

			if (sMNPSync->bWindowSize == 0)
				iPredictedWindow += 0;
			else
				iPredictedWindow += 5140 + (int(sMNPSync->bWindowSize / 2.346) + 1) * 244;

			if (s01->mNet->bDeviceId[0] == sMNPSync->bDeviceId[0] &&
				s01->mNet->bDeviceId[1] == sMNPSync->bDeviceId[1] &&
				s01->mNet->bDeviceId[2] == sMNPSync->bDeviceId[2] &&
				s01->mNet->bDeviceId[3] == sMNPSync->bDeviceId[3])
			{
				iOurSlot = iNodes;
			}

			//print("Device %d: Id %2.2X %2.2X %2.2X %2.2X Window size: %2.2d.%s\n", iNodes, sMNPSync->bDeviceId[0], sMNPSync->bDeviceId[1], sMNPSync->bDeviceId[2], sMNPSync->bDeviceId[3], sMNPSync->bWindowSize, (iOurSlot == -1 ? "" : " *US*"));
		}


#define LARGESTEP 750
#define SMALLSTEP 10

		if (!s01->bPaused)
		{
			s01->mNet->bDeviceId[3] = s01->iTWindowCount;

			if (s01->eSS == SS_Cleanup)
			{ //remove all nodes.
				if (iNodes == 1)
				{
					print("----cleanup done.\n");
					s01->eSS = SS_Setup;
					return;
				}
				
				uint64_t tWindow = p->tTime + iPredictedWindow;
				s01->mNet->bDeviceId[3] = iNodes;
				print("----trying to remove node 1:1:1:%x.\n", iNodes);

				s01->mNet->Send(0x04, NULL, 0, tWindow);

			}
			else if (s01->eSS == SS_Setup)
			{ //add nodes
				if (iNodes == s01->iTWindowCount)
				{
					print("----setup done.\n");
					s01->iDelay = iPredictedWindow;
					s01->eSS = SS_Window;
					return;
				}

				int iCurrentWindowSize = s01->iTWindows[iNodes - 1];
				uint64_t tWindow = p->tTime + iPredictedWindow;
				s01->mNet->bDeviceId[3] = iNodes + 1;
				print("----trying to add node 1:1:1:%x with window %d.\n", iNodes + 1, iCurrentWindowSize);

				byte b[3] = {0x00, iCurrentWindowSize, iCurrentWindowSize};
				s01->mNet->Send(0x03, b, 3, tWindow);
			}


			
			else if (s01->eSS == SS_Window)
			{
				if (iNodes < s01->iTWindowCount + 1 && s01->bLastActionAdding == false)
				{
					print("----trying to add probing node. %d nodes\n", iNodes);
					
					s01->bLastActionAdding = true;
					s01->mNet->bDeviceId[3] = iNodes + 1;
					uint64_t tWindow = p->tTime + s01->iDelay;
					byte b[3] = {0x00, 0, 0};
					s01->mNet->Send(0x03, b, 3, tWindow);

					s01->iDelay -= SMALLSTEP;
				}

				else if (iNodes < s01->iTWindowCount + 1 && s01->bLastActionAdding == true)
				{//missing feedback. note it.
					s01->iMissed++;
					if (s01->iMissed > 10)
					{
						print("\t%d\t%d\n", iNodes, s01->iWorkingDelayCurrentNode);
						//s01->iDelay = s01->iWorkingDelayCurrentNode - 500;
						//s01->iMissed = 0;
						//s01->iCurrentWindowSize++;
						//s01->eSS = SS_Window;
						s01->bPaused = true;
						return;
						//print("----start found at %d\n", s01->iWorkingDelayCurrentNode);
					}
					else
					{
						s01->bLastActionAdding = true;
						s01->mNet->bDeviceId[3] = iNodes + 1;
						uint64_t tWindow = p->tTime + s01->iDelay;
						byte b[3] = {0x00, 0, 0};
						s01->mNet->Send(0x03, b, 3, tWindow);
					}					
				}
				
				else if (iNodes == s01->iTWindowCount + 1)
				{//got feedback. remove probing node and keep searching.
					print("----trying to remove probing node. %d nodes.\n", iNodes);
					s01->iMissed = 0;
					s01->iWorkingDelayCurrentNode = s01->iDelay;
					s01->bLastActionAdding = false;

					s01->mNet->bDeviceId[3] = iNodes;
					uint64_t tWindow = p->tTime + iPredictedWindow;
					s01->mNet->Send(0x04, NULL, 0, tWindow);
				}
				


			}
			else if (s01->eSS == SS_WindowEnd)
			{

			}
		}

	}	
}
