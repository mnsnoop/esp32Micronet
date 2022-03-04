#ifndef _BPRINT_H
#define _BPRINT_H

#include "Arduino.h"

#define PRINTBUFFERSIZE 512 //max number of messages
#define PRINTMESSAGESIZE 512 //max size of a message
#define PRINTMAXSERIALWRITE 10 //max chars to write to the serial port before task yielding

enum _eLogLevel
{
	LL_NONE,
	LL_ERROR,
	LL_WARN,
	LL_INFO,
	LL_DEBUG,
	LL_VERBOSE	
};

static const char *_cLogLevel[] =
{
	"   ",
	"ERR",
	"WAR",
	"INF",
	"DBG",
	"VRB"
};

/*
* NOTE! print() is not ISR safe! Printing from within an ISR will lead to stablity problems. Useful in debugging but don't leave in production.
*/

void PrintInitialize();
void print(_eLogLevel ell, char *format, ...); //prints with a timestamp.
void printnts(_eLogLevel ell, char *format, ...); //prints without the timestamp.
void fastprint(_eLogLevel ell, const char *c, int size); //prints without the timestamp or formatting. Mostly for ISRs.
void tfPrint(void *p);
void SetLogLevel(_eLogLevel ell);

#endif // _BPRINT_H
