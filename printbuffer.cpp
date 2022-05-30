#include "printbuffer.h"

TaskHandle_t tPrinter;

struct sPrintMessage
{
	int iSize;
	char *cMessage;
};

QueueHandle_t qPrintBuffer = xQueueCreate(PRINTBUFFERSIZE, sizeof(sPrintMessage));

_eLogLevel eLogLevel = LL_NONE;

void PrintInitialize()
{
	xTaskCreatePinnedToCore(tfPrint, "printer", configMINIMAL_STACK_SIZE+500, NULL, tskIDLE_PRIORITY, &tPrinter, 0);	
}

void print(_eLogLevel ell, bool bTS, const char *format, va_list args)
{
	char cTimestamp[20];
	char cLocalBuffer[PRINTMESSAGESIZE];
	int iTSSize = 0;

static const char *_cLogLevel[] =
{
	"   ",
	"ERR",
	"WAR",
	"INF",
	"DBG",
	"VRB"
};

	if (bTS)
	{
		uint64_t tCurrentTick = esp_timer_get_time();
		int iSeconds = tCurrentTick / 1000000;
		int iUSeconds = tCurrentTick - (iSeconds * 1000000);
		iTSSize = snprintf(cTimestamp, 20, "%4.4d.%6.6d:%.3s ", iSeconds, iUSeconds, _cLogLevel[ell]);
		strncpy(cLocalBuffer, cTimestamp, 16);
	}

	int iMessageSize = vsnprintf(cLocalBuffer + iTSSize, PRINTMESSAGESIZE - iTSSize, format, args);


	sPrintMessage sPM;
	sPM.iSize = iMessageSize + iTSSize;
	sPM.cMessage =(char *)malloc(sPM.iSize * sizeof(char));
	memcpy(sPM.cMessage, cLocalBuffer, sPM.iSize);

	if (xQueueSendToBackFromISR(qPrintBuffer, (const void *)&sPM, NULL) != pdPASS)
		free(sPM.cMessage);
}

void fastprint(_eLogLevel ell, const char *c, int size)
{
	sPrintMessage sPM;
	sPM.iSize = size;
	sPM.cMessage =(char *)malloc(size * sizeof(char));
	memcpy(sPM.cMessage, c, size);

	if (xQueueSendToBackFromISR(qPrintBuffer, (const void *)&sPM, NULL) != pdPASS)
		free(sPM.cMessage);
}

void print(_eLogLevel ell, const char *format, ...)
{
	if (ell > eLogLevel)
	return;

	va_list args;
	va_start(args, format);
	print(ell, true, format, args);
	va_end(args);
}

void printnts(_eLogLevel ell, const char *format, ...)
{
	if (ell > eLogLevel)
	return;

	va_list args;
	va_start(args, format);
	print(ell, false, format, args);
	va_end(args);
}

void tfPrint(void *p)
{
	sPrintMessage sPM;

	for (;;)
	{
		if (xQueueReceive(qPrintBuffer, (void *)&sPM, portMAX_DELAY) == pdTRUE)
		{
			int i = 0;
			for (; i < sPM.iSize - PRINTMAXSERIALWRITE; i += PRINTMAXSERIALWRITE)
			{
				Serial.write((char *)sPM.cMessage + i, PRINTMAXSERIALWRITE);
				taskYIELD();
			}
			Serial.write((char *)sPM.cMessage + i, sPM.iSize - i);

			free(sPM.cMessage);
		}
	}
}

void SetLogLevel(_eLogLevel ell)
{
	eLogLevel = ell;
}
