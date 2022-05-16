/*
 * LSM303DLHCDriver.cpp
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#include "BoardConfig.h"
#include "LSM303DLHCDriver.h"
#include <Wire.h>

LSM303DLHCDriver::LSM303DLHCDriver() :
		accAddr(LSM303DLHC_ACC_ADDR), magAddr(LSM303DLHC_MAG_ADDR), magX(0), magY(0), magZ(0), accX(0), accY(0), accZ(0)
{
}

LSM303DLHCDriver::~LSM303DLHCDriver()
{
}

bool LSM303DLHCDriver::Init()
{
	uint8_t ira, irb, irc, sr, whoami;

	NAVCOMPASS_I2C.begin();

	if (!I2CRead(LSM303DLHC_ACC_ADDR, STATUS_REG_A, &sr))
	{
		return false;
	}
	accAddr = LSM303DLHC_ACC_ADDR;

	I2CRead(LSM303DLHC_MAG_ADDR, IRA_REG_M, &ira);
	I2CRead(LSM303DLHC_MAG_ADDR, IRB_REG_M, &irb);
	I2CRead(LSM303DLHC_MAG_ADDR, IRC_REG_M, &irc);

	if ((ira != 'H') || (irb != '4') || (irc != '3'))
	{
		return false;
	}

	magAddr = LSM303DLHC_MAG_ADDR;

	I2CRead(magAddr, WHO_AM_I_M, &whoami);

	if (whoami != LSM303DLHC_WHO_AM_I)
	{
		return false;
	}

	// Acceleration register
	I2CWrite(accAddr, 0x47, CTRL_REG1_A); // 0x47=0b01000111 Normal Mode, ODR 50Hz, all axes on
//	I2CWrite(accAddr, 0x57, CTRL_REG1_A); // 0x57=0b01010111 Normal Mode, ODR 100Hz, all axes on
	I2CWrite(accAddr, 0x08, CTRL_REG4_A); // 0x08=0b00001000 Range: +/-2 Gal, Sens.: 1mGal/LSB, highRes on
//	I2CWrite(accAddr, 0x18, CTRL_REG4_A); // 0x18=0b00011000 Range: +/-4 Gal, Sens.: 2mGal/LSB, highRes on

	// Magnetic register
	I2CWrite(magAddr, 0x10, CRA_REG_M); // 0x10=0b00010000 ODR 15Hz
//	I2CWrite(magAddr, 0x90, CRA_REG_M); // 0x90=0b10010000 ODR 15Hz, temperature sensor on
//	I2CWrite(magAddr, 0x14, CRA_REG_M); // 0x14=0b00010100 ODR 30Hz
//	I2CWrite(magAddr, 0x98, CRA_REG_M); // 0x18=0b10011000 ODR 75Hz, temperature sensor on
	I2CWrite(magAddr, 0x20, CRB_REG_M); // 0x20=0b00100000 Range: +/-1.3 Gauss gain: 1100LSB/Gauss
//	I2CWrite(magAddr, 0x60, CRB_REG_M); // 0x60=0b01100000 Range: +/-2.5 Gauss gain: 670LSB/Gauss

	I2CWrite(magAddr, 0x00, MR_REG_M); // Continuous mode

	return true;
}

string LSM303DLHCDriver::GetDeviceName()
{
	return (string("LSM303DLHC"));
}

void LSM303DLHCDriver::GetMagneticField(float *magX, float *magY, float *magZ)
{
	uint8_t magBuffer[6];
	int16_t mx, my, mz;

	I2CBurstRead(magAddr, OUT_X_H_M, magBuffer, 6);

	mx = ((int16_t) (magBuffer[0] << 8)) | magBuffer[1];
	mz = ((int16_t) (magBuffer[2] << 8)) | magBuffer[3]; // stupid change in order for DLHC
	my = ((int16_t) (magBuffer[4] << 8)) | magBuffer[5];

	*magX = (float) mx;
	*magY = (float) my;
	*magZ = (float) mz;
}

void LSM303DLHCDriver::GetAcceleration(float *accX, float *accY, float *accZ)
{
	int16_t ax, ay, az;
	uint8_t regValue;

	I2CRead(accAddr, OUT_X_H_A, &regValue);
	ax = regValue;
	I2CRead(accAddr, OUT_X_L_A, &regValue);
	ax = (ax << 8) | regValue;

	I2CRead(accAddr, OUT_Y_H_A, &regValue);
	ay = regValue;
	I2CRead(accAddr, OUT_Y_L_A, &regValue);
	ay = (ay << 8) | regValue;

	I2CRead(accAddr, OUT_Z_H_A, &regValue);
	az = regValue;
	I2CRead(accAddr, OUT_Z_L_A, &regValue);
	az = (az << 8) | regValue;

	*accX = (float) (ax >> 4); // DLHC registers contain a left-aligned 12-bit number, so values should be shifted right by 4 bits (divided by 16)
	*accY = (float) (ay >> 4);
	*accZ = (float) (az >> 4);
}

bool LSM303DLHCDriver::I2CRead(uint8_t i2cAddress, uint8_t address, uint8_t *data)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	if (NAVCOMPASS_I2C.endTransmission() != 0)
	{
		return false;
	}
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) 1);
	*data = NAVCOMPASS_I2C.read();
	NAVCOMPASS_I2C.endTransmission();

	return (NAVCOMPASS_I2C.endTransmission() == 0);
}

bool LSM303DLHCDriver::I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	if (NAVCOMPASS_I2C.endTransmission() != 0)
	{
		return false;
	}
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) length);
	NAVCOMPASS_I2C.readBytes(buffer, NAVCOMPASS_I2C.available());
	return (NAVCOMPASS_I2C.endTransmission() == 0);
}

bool LSM303DLHCDriver::I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.write(data);
	return (NAVCOMPASS_I2C.endTransmission() == 0);
}
