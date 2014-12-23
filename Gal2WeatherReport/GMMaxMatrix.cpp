/*
* GMMaxMatrix
* Version 1.0 Feb 2013
* Copyright 2013 Oscar Kin-Chung Au

Modified by GigaMegaWatts
*/


#include "Arduino.h"
#include "GMMaxMatrix.h"
#include <SPI.h>

GMMaxMatrix::GMMaxMatrix(//byte _data, 
	byte _load,
	//byte _clock, 
	byte _num)
{
	//data = _data;
	load = _load;
	//clock = _clock;
	numMatrixes = _num;
	for (int i = 0; i<BITMAP_BUFFER_LEN; i++)
		buffer[i] = 0;
}

void GMMaxMatrix::init()
{
	//pinMode(data,  OUTPUT);
	//pinMode(clock, OUTPUT);
	pinMode(load, OUTPUT);
	//digitalWrite(clock, HIGH); 


	//digitalWrite(load, HIGH);

	SPI.begin();
	//SPI.setBitOrder(MSBFIRST);

	// DAW - 10/18/14 - try slower speed on Galileo to prevent intermittent blank led matrixes
	// - removed: slowest divider that works is DIV16, and that didn't fix the problem
	//SPI.setClockDivider(SPI_CLOCK_DIV16);
	//SPI.setDataMode(SPI_MODE0);

	setCommand(max7219_reg_scanLimit, 0x07);
	setCommand(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)

	setCommand(max7219_reg_displayTest, 0x00); // no display test
	setCommand(max7219_reg_shutdown, 0x01);    // not in shutdown mode	
	// empty registers, turn all LEDs off
	clear();

	//setIntensity(0x02);    // the first 0x0f is the value you can set
}


void GMMaxMatrix::setIntensity(byte intensity)
{
	setCommand(max7219_reg_intensity, intensity);
}

void GMMaxMatrix::clear()
{
	for (int i = 0; i<8; i++)
		setColumnAll(i, 0);

	for (int i = 0; i<BITMAP_BUFFER_LEN; i++)
		buffer[i] = 0;
}

void GMMaxMatrix::setCommand(byte command, byte value)
{
	//digitalWrite(load, LOW);    
	//for (int i=0; i<numMatrixes; i++) 
	//{
	//	shiftOut(data, clock, MSBFIRST, command);
	//	shiftOut(data, clock, MSBFIRST, value);
	//}
	//digitalWrite(load, LOW);
	//digitalWrite(load, HIGH);
	//digitalWrite(load, LOW);
	//for (int i = 0; i < numMatrixes; i++)
	//{
	//	tempBuffer[i * 2] = command;
	//	tempBuffer[i*2 + 1] = value;
	//}
	digitalWrite(load, LOW);
	//SPI.transferBuffer(tempBuffer, NULL, numMatrixes * 2);
	for (int i = 0; i < numMatrixes; i++)
	{
		SPI.transfer(command);
		SPI.transfer(value);
	}
	digitalWrite(load, HIGH);

}

///col = 0-indexed
void GMMaxMatrix::setColumn(byte col, byte value)
{
	int n = col / 8;
	int c = col % 8;

#if DEBUG		
	Serial.print("setColumn, col=");
	Serial.print(col, DEC);
	Serial.print(", value=");
	Serial.print(value, DEC);
	Serial.print(", n=");
	Serial.print(n);
	Serial.print(", c=");
	Serial.println(c);
#endif

	// build an array of the byte values to send
	int bufferIndex = 0;
	for (int i = 0; i<numMatrixes; i++)
	{
		if (i == n)
		{
			// this is the matrix we want to update
			tempBuffer[bufferIndex++] = c + 1;
			tempBuffer[bufferIndex++] = value;
		}
		else
		{
			// leave this matrix unchanged - set register and value to 0s
			tempBuffer[bufferIndex++] = 0;
			tempBuffer[bufferIndex++] = 0;
		}
	}
	//digitalWrite(load, LOW);


	digitalWrite(load, LOW);
#if DEBUG	
	Serial.print("SPI buffer len=");
	Serial.println(bufferIndex);
	Serial.print("SPI transferBuffer:");
#endif
	for (int i = 0; i < bufferIndex; i++)
	{
#if DEBUG	
		Serial.print(tempBuffer[i], HEX);
		Serial.print(",");
#endif
		SPI.transfer(tempBuffer[i]);
	}
#if DEBUG	
	Serial.println("");
#endif
	//SPI.transferBuffer(tempBuffer, NULL, bufferIndex);
	digitalWrite(load, HIGH);

	buffer[col] = value;
}

void GMMaxMatrix::setColumnAll(byte col, byte value)
{

	// build an array of the byte values to send
	int bufferIndex = 0;
	for (int i = 0; i<numMatrixes; i++)
	{
		//shiftOut(data, clock, MSBFIRST, col + 1);
		//shiftOut(data, clock, MSBFIRST, value);
		tempBuffer[bufferIndex++] = col + 1;
		tempBuffer[bufferIndex++] = value;
		buffer[col * i] = value;
	}

	digitalWrite(load, LOW);
	//SPI.transferBuffer(tempBuffer, rxbuffer, bufferIndex);
	for (int i = 0; i < bufferIndex; i++)
	{
		//Serial.print(tempBuffer[i], HEX);
		SPI.transfer(tempBuffer[i]);
	}
	digitalWrite(load, HIGH);
}

void GMMaxMatrix::setDot(byte col, byte row, byte value)
{
	bitWrite(buffer[col], row, value);

	int n = col / 8;
	int c = col % 8;
	// build an array of the byte values to send
	int bufferIndex = 0;
	for (int i = 0; i<numMatrixes; i++)
	{
		if (i == n)
		{
			// this is the matrix we want to update
			tempBuffer[bufferIndex++] = c + 1;
			tempBuffer[bufferIndex++] = buffer[col];
		}
		else
		{
			// leave this matrix unchanged - set register and value to 0s
			tempBuffer[bufferIndex++] = 0;
			tempBuffer[bufferIndex++] = 0;
		}
	}
	//SPI.transferBuffer(tempBuffer, rxbuffer, bufferIndex);
	for (int i = 0; i < bufferIndex; i++)
	{
		//Serial.print(tempBuffer[i], HEX);
		SPI.transfer(tempBuffer[i]);
	}
}

void GMMaxMatrix::writeSprite(int x, int y, const byte* sprite)
{
	//Serial.println("in writeSprite");
	int w = sprite[0];
	int h = sprite[1];

	if (h == 8 && y == 0)
		for (int i = 0; i<w; i++)
		{
		int c = x + i;
		if (c >= 0 && c<BITMAP_BUFFER_LEN)
			setColumn(c, sprite[i + 2]);
		}
	else
		for (int i = 0; i<w; i++)
			for (int j = 0; j<h; j++)
			{
		int c = x + i;
		int r = y + j;
		if (c >= 0 && c<BITMAP_BUFFER_LEN && r >= 0 && r<8)
			setDot(c, r, bitRead(sprite[i + 2], j));
			}
	//Serial.println("exiting writeSprite");
}

void GMMaxMatrix::reload()
{

	for (int i = 0; i< numMatrixes * 8; i++)
	{
		setColumn(i, buffer[i]);
	}
}

void GMMaxMatrix::shiftLeft(bool rotate, bool fill_zero)
{
	byte old = buffer[0];
	int i;
	for (i = 0; i<BITMAP_BUFFER_LEN; i++)
		buffer[i] = buffer[i + 1];
	if (rotate) buffer[numMatrixes * 8 - 1] = old;
	else if (fill_zero) buffer[numMatrixes * 8 - 1] = 0;

	reload();
}

void GMMaxMatrix::shiftRight(bool rotate, bool fill_zero)
{
	int last = numMatrixes * 8 - 1;
	byte old = buffer[last];
	int i;
	for (i = 79; i>0; i--)
		buffer[i] = buffer[i - 1];
	if (rotate) buffer[0] = old;
	else if (fill_zero) buffer[0] = 0;

	reload();
}

void GMMaxMatrix::shiftUp(bool rotate)
{
	for (int i = 0; i<numMatrixes * 8; i++)
	{
		bool b = buffer[i] & 1;
		buffer[i] >>= 1;
		if (rotate) bitWrite(buffer[i], 7, b);
	}
	reload();
}

void GMMaxMatrix::shiftDown(bool rotate)
{
	for (int i = 0; i<numMatrixes * 8; i++)
	{
		bool b = buffer[i] & 128;
		buffer[i] <<= 1;
		if (rotate) bitWrite(buffer[i], 0, b);
	}
	reload();
}


