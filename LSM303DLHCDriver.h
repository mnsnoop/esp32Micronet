/*
 * LSM303DLHCDriver.h
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#ifndef LSM303DLHCDRIVER_H_
#define LSM303DLHCDRIVER_H_

#include "NavCompassDriver.h"

#define LSM303DLHC_MAG_ADDR   0x1E
#define LSM303DLHC_ACC_ADDR   0x19

#define LSM303DLHC_WHO_AM_I 0x3C

#define CTRL_REG1_A       0x20
#define CTRL_REG2_A       0x21
#define CTRL_REG3_A       0x22
#define CTRL_REG4_A       0x23
#define CTRL_REG5_A       0x24
#define CTRL_REG6_A       0x25
#define REFERENCE_A       0x26
#define STATUS_REG_A      0x27
#define OUT_X_L_A         0x28
#define OUT_X_H_A         0x29
#define OUT_Y_L_A         0x2a
#define OUT_Y_H_A         0x2b
#define OUT_Z_L_A         0x2c
#define OUT_Z_H_A         0x2d
#define FIFO_CTRL_REG_A   0x2e
#define FIFO_SRC_REG_A    0x2f
#define INT1_CFG_A        0x30
#define INT1_SOURCE_A     0x31
#define INT1_THS_A        0x32
#define INT1_DURATION_A   0x33
#define INT2_CFG_A        0x34
#define INT2_SOURCE_A     0x35
#define INT2_THS_A        0x36
#define INT2_DURATION_A   0x37
#define CRA_REG_M         0x00
#define CRB_REG_M         0x01
#define MR_REG_M          0x02
#define OUT_X_H_M         0x03
#define OUT_X_L_M         0x04
#define OUT_Y_H_M         0x05
#define OUT_Y_L_M         0x06
#define OUT_Z_H_M         0x07
#define OUT_Z_L_M         0x08
#define SR_REG_M          0x09
#define IRA_REG_M         0x0a
#define IRB_REG_M         0x0b
#define IRC_REG_M         0x0c
#define WHO_AM_I_M        0x0f
#define TEMP_OUT_H_M      0x31
#define TEMP_OUT_L_M      0x32

using string = std::string;

class LSM303DLHCDriver: public NavCompassDriver
{
public:
	LSM303DLHCDriver();
	virtual ~LSM303DLHCDriver();

	virtual bool Init();
	virtual string GetDeviceName();
	virtual void GetMagneticField(float *magX, float *magY, float *magZ);
	virtual void GetAcceleration(float *accX, float *accY, float *accZ);

private:
	uint8_t accAddr, magAddr;
	float magX, magY, magZ;
	float accX, accY, accZ;

	bool I2CRead(uint8_t i2cAddress, uint8_t address, uint8_t *data);
	bool I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length);
	bool I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address);
};

#endif /* LSM303DLHDRIVER_H_ */
