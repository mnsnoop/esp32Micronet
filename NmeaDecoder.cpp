/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Decode data from Micronet devices send it on an NMEA network  *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <Arduino.h>

#include "BoardConfig.h"
#include "micronet.h"
#include "Globals.h"
#include "NmeaDecoder.h"
#include "NavCompass.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Macros                                   */
/***************************************************************************/

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

NmeaDecoder::NmeaDecoder()
{
	writeIndex = 0;
	serialBuffer[0] = 0;
}

NmeaDecoder::~NmeaDecoder()
{
}

void IRAM_ATTR NmeaDecoder::PushChar(char c)
{
	if ((serialBuffer[0] != '$') || (c == '$'))
	{
		serialBuffer[0] = c;
		writeIndex = 1;
		return;
	}

	if (c == 13) //CR
	{
		if (writeIndex >= 10)
		{
			memcpy(sentenceBuffer, serialBuffer, writeIndex);
			sentenceBuffer[writeIndex] = 0;
			DecodeSentence(sentenceBuffer);
		}
		else
		{
			serialBuffer[0] = 0;
			writeIndex = 0;
			return;
		}
	}

	serialBuffer[writeIndex++] = c;

	if (writeIndex >= NMEA_SENTENCE_MAX_LENGTH)
	{
		serialBuffer[0] = 0;
		writeIndex = 0;
		return;
	}
}

void IRAM_ATTR NmeaDecoder::DecodeSentence(char *sentence)
{
	if (sentence[0] != '$')
		return;

	char *pCs = strrchr(sentence, '*') + 1;
	if (pCs == nullptr)
		return;
	int16_t Cs = (NibbleValue(pCs[0]) << 4) | NibbleValue(pCs[1]);
	if (Cs < 0)
		return;

	uint8_t crc = 0;
	for (char *pC = sentence + 1; pC < (pCs - 1); pC++)
	{
		crc = crc ^ (*pC);
	}

	if (crc != Cs)
		return;

	uint32_t sId = ((uint8_t) sentence[3]) << 16;
	sId |= ((uint8_t) sentence[4]) << 8;
	sId |= ((uint8_t) sentence[5]);

	char *pField = sentence + 7;
	switch (sId)
	{
	case 0x524D42:
		// RMB sentence
		DecodeRMBSentence(pField);
		break;
	case 0x524D43:
		// RMC sentence
		DecodeRMCSentence(pField);
		break;
	case 0x474741:
		// GGA sentence
		DecodeGGASentence(pField);
		break;
	case 0x565447:
		// VTG sentence
		DecodeVTGSentence(pField);
		break;
	}

	float heading = gNavCompass.GetHeading();
	mNet.SetHeading(heading);

	return;
}

void IRAM_ATTR NmeaDecoder::DecodeRMBSentence(char *sentence)
{
	float value;

	if (sentence[0] != 'A')
	{
		return;
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		mNet.SetXTE(value);
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'R')
		mNet.SetXTE(-value);
	for (int i = 0; i < 7; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		mNet.SetDTW(value);
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		mNet.SetBTW(value);
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		mNet.SetSpeed(value);
	}
}

void IRAM_ATTR NmeaDecoder::DecodeRMCSentence(char *sentence)
{
	if (sentence[0] != ',')
	{
		uint8_t iHour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		uint8_t iMinute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		mNet.SetTime(iHour, iMinute);
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		uint8_t iDay = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		uint8_t iMonth = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		uint8_t iYear = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		mNet.SetDate(iDay, iMonth, iYear);
	}
}

void IRAM_ATTR NmeaDecoder::DecodeGGASentence(char *sentence)
{
	float degs, mins, fLat=0.0, fLong=0.0;

	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;

	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		sscanf(sentence + 2, "%f,", &mins);
		fLat = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'S')
			fLat = -fLat;
	}

	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
		sscanf(sentence + 3, "%f,", &mins);
		fLong = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'W')
			fLong = -fLong;
	}
	mNet.SetLatLong(fLat, fLong);
}

void IRAM_ATTR NmeaDecoder::DecodeVTGSentence(char *sentence)
{
	float value, fSOG=0.0, fCOG=0.0;

	if (sscanf(sentence, "%f", &value) == 1)
	{
		fSOG = value;
	}
	for (int i = 0; i < 4; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		fCOG = value;
	}
	mNet.SetSOGCOG(fSOG, fCOG);
}

int16_t IRAM_ATTR NmeaDecoder::NibbleValue(char c)
{
	if ((c >= '0') && (c <= '9'))
	{
		return (c - '0');
	}
	else if ((c >= 'A') && (c <= 'F'))
	{
		return (c - 'A') + 0x0a;
	}
	else if ((c >= 'a') && (c <= 'f'))
	{
		return (c - 'a') + 0x0a;
	}

	return -1;
}
