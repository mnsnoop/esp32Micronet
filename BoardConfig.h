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

#ifndef BOARDCONFIG_H_
#define BOARDCONFIG_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

//#define HAS_N2KBRIDGE
//#define HAS_ANEMOMETER
//#define HAS_PADDLEWHEEL
#define HAS_GNSS
#define HAS_COMPASS

// Selects on which I2C bus is connected compass as per Wiring library definition
#define NAVCOMPASS_I2C Wire
#define I2C_SDA       21 // default pins
#define I2C_SCL       22

//#define CC1000
#define CC1101

#ifdef CC1000
//CC1000 Settings
#define PIN_DDATA	19
#define PIN_DCLOCK	23
#define PIN_PCLOCK	18
#define PIN_PDATA	5
#define PIN_PALE	2
#endif
#ifdef CC1101
//CC1101 Settings
#define PIN_MISO	19
#define PIN_MOSI	23
#define PIN_SCLK	18
#define PIN_CS		5
#define PIN_G0		2
#endif

// ERROR LED pin
//#define LED_PIN     2

// NMEA/GNSS UART pins
// USB-Serial0 RX0: GPIO3 TX0: GPIO1; Serial1 RX1: GPIO09 TX1: GPIO10; Serial2 RX2: GPIO16 TX2: GPIO17
#define GNSS_SERIAL   Serial2
#define GNSS_BAUDRATE 115200
#define GNSS_CALLBACK serialEvent2
#define GNSS_RX_PIN   16
#define GNSS_TX_PIN   17

// USB UART params
#define USB_CONSOLE  Serial
#define USB_BAUDRATE 115200

// The console to use for menu
#define CONSOLE USB_CONSOLE

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
