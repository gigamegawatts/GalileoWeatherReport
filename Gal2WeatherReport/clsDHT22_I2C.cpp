#include "clsDHT22_I2C.h"




clsDHT22_I2C::clsDHT22_I2C()
{
}

boolean clsDHT22_I2C::begin(void)
{
	
	Wire.begin();
	
	return isAvailable();
}

boolean clsDHT22_I2C::isAvailable(void)
{
	uint8_t statusVal;
	// register 6 should always return the I2C address
	statusVal = read8(status);
	if (statusVal == DHT22_I2C_ADDR)
	{
		return true;
	}
	else
	{
		return false;
	}
}

float clsDHT22_I2C::getTemperature()
{
	uint8_t temp_hi;
	uint8_t temp_lo;
	float result;

	// temperature is returned in 2 registers.  MSB register's 1st bit is 1 if negative.  Result is divided by 10 to get C temperature.
	temp_hi = read8(temp_msb);
	temp_lo = read8(temp_lsb);
	// strip out 1st bit of MSB - it is a positive/minus indicator
	// divide by 10, since temperature is returned with 1 implicit decimal place
	result = ((temp_hi & 0x7f) * 256 + temp_lo) / 10.0;
	// determine if temperature is plus or minus
	if (temp_hi & 80)
	{
		result = -1 * result;
	}
	return result;
}

uint8_t clsDHT22_I2C::getNumReadings()
{
	return read8(num_readings);
}

uint8_t clsDHT22_I2C::getNumErrors()
{
	return read8(num_errors);
}

//clsDHT22_I2C::~clsDHT22_I2C()
//{
//}

// NOTE - the following 3 functions are taken from the Adafruit BMP085 Arduino Library
//  https://github.com/adafruit/Adafruit-BMP085-Library

uint8_t clsDHT22_I2C::read8(clsDHT22_I2C::DHT22Register regAddr) {
	uint8_t ret;

	Wire.beginTransmission(DHT22_I2C_ADDR); // start transmission to device 

	Wire.write((uint8_t)regAddr); // sends register address to read from

	Wire.endTransmission(); // end transmission

	// DAW - removed: causes I2C_CONTROLLER error on Win Galileo
	//Wire.beginTransmission(BMP085_I2CADDR); // start transmission to device 
	Wire.requestFrom(DHT22_I2C_ADDR, 1);// send data n-bytes read

	ret = Wire.read(); // receive DATA

	// DAW - removed: causes I2C_CONTROLLER error on Win Galileo
	//Wire.endTransmission(); // end transmission

	return ret;
}

uint16_t clsDHT22_I2C::read16(uint8_t regAddr) {
	uint16_t ret;

	Wire.beginTransmission(DHT22_I2C_ADDR); // start transmission to device 
	Wire.write(regAddr); // sends register address to read from

	Wire.endTransmission(); // end transmission

	// DAW - removed: causes I2C_CONTROLLER error on Win Galileo
	//Wire.beginTransmission(BMP085_I2CADDR); // start transmission to device 
	Wire.requestFrom(DHT22_I2C_ADDR, 2);// send data n-bytes read

	ret = Wire.read(); // receive DATA
	ret <<= 8;
	ret |= Wire.read(); // receive DATA

	//Wire.endTransmission(); // end transmission

	return ret;
}

void clsDHT22_I2C::write8(uint8_t regAddr, uint8_t data) {
	Wire.beginTransmission(DHT22_I2C_ADDR); // start transmission to device 
	Wire.write(regAddr); // sends register address to read from
	Wire.write(data);  // write data
	Wire.endTransmission(); // end transmission
}
