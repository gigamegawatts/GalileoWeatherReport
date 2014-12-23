#pragma once

#include <Wire.h>
#include <Arduino.h>

#define DHT22_I2C_ADDR 9
class clsDHT22_I2C
{
public:

	clsDHT22_I2C();
	//~clsDHT22_I2C();
	boolean begin(void);
	boolean isAvailable(void);
	float getTemperature(void);
	uint8_t getNumReadings(void);
	uint8_t getNumErrors(void);


private:

	// register map:
	// 0 = num valid readings since last request (if > 1, then temperature and humidity are averaged)
	// 1 = error count since last request
	// 2 = temperature  - MSB: NOTE- implicit 1 decimal place, negative is 1st bit is 1
	// 3 = temperature - LSB
	// 4 = humidity - MSB: NOTE- implicit 1 decimal place
	// 5 = humidity - LSB
	// 6 = device ID (same as I2C address)

	enum DHT22Register {
		num_readings, num_errors, temp_msb, temp_lsb, hum_msb, hum_lsb, status
	};

	uint8_t read8(DHT22Register regAddr);
	uint16_t read16(uint8_t regAddr);
	void write8(uint8_t regAddr, uint8_t data);
};

