// Main.cpp : Defines the entry point for the console application.
//
#define _WINSOCKAPI_  // DAW - on Gal2, prevent inclusion of winsock.h
#include "stdafx.h"
#include "arduino.h"
#include "DateTime.h"
#include "GMBmp085.h"
#include "TH02_dev.h"
#include "clsDHT22_I2C.h"
#include "GMCosm.h"
#include "GMWunderground.h"
#include "GMMaxMatrix.h"

#include "fstream"
#include "iostream"
using namespace std;

// temperature sensor type:
//#define USE_BMP085
//#define USE_TH02
//#define USE_DHT22_I2C
#define USE_DHT22_SERIAL

int readCPUTemp();
void printString(char* s);
int readIni(char * filename);
string delSpaces(string &str);
void getTempAndHumidityFromSerial();
#ifdef USE_DHT22_SERIAL

void copyToBuffer(char *target, int intMaxLen);
int fillBuffer();
#endif

//---------------- USER CONFIGURABLE SETTINGS --------------------
// ------------------- Cosm (Xively) settings ---------------
// If 1, writes output to serial UART rather than dot matrix.  If 2, writes current conditions to Serial and forecast to Serial1
#define WRITE_TO_SERIAL 2
// NOTE - these can also be put in the WeatherReport.ini file on your Galileo
#define COSM_API_KEY ""
#define COSM_FEED ""
#define COSM_CPU_TEMP_DATASTREAM ""
#define COSM_TEMP_DATASTREAM ""
#define COSM_PRESSURE_DATASTREAM ""
#define COSM_HUMIDITY_DATASTREAM ""
// upload interval, in ms
#define COSM_UPLOAD_INTERVAL 300*1000 
// ------------------ measurement settings -------------------
// interval at which to take new measurements, in ms
#define MEASUREMENT_INTERVAL 30*1000
#define WEATHER_MEASUREMENT_INTERVAL 3600*1000 //600*1000 // get outside weather every 10 minutes
#define FORECAST_MEASUREMENT_INTERVAL 18000*1000 //3600*1000 // get forecast every hour

// --------------- LED matrix settings ---------------------
// number of 8x8 matrixes
#define NUM_MATRIX 2
// pin connected to matrix CS
#define MATRIX_CS_PIN 10
// max characters to be written to matrix (including ones that scroll by)
// NOTE - this should be <= Arduino's BUFFER_LEN
// FOLLOWING is for GMMaxMatrix on Arduino
//NOTE!!!!!!!!!!!!!!!!!!!! - a buffer this size won't work with the default Arduino RX buffer of 64 bytes.  
//  - MUST increase it in HardwareSerial.h (Arduino 1.x) or HardwareSerial.cpp (Arduino 1.5.x)
#define MATRIX_DATA_LEN 120
// FOLLOWING is for JY-MCU 3208
//#define MATRIX_DATA_LEN 200
// # of ms to wait before scrolling a column: the lower the #, the faster it scrolls
#define MATRIX_SCROLL_MS 25
// HACK - workaround for intermittent problem where one matrix goes blank
// how often to reset the display - after n interations of the message, it will be reset (with very short pause)
#define MATRIX_RESET_FREQUENCY 25
// ------------------ Weather Underground settings ------------------
#define WUND_API ""
#define WUND_CITY ""
#define WUND_COUNTRY ""
// number of forecasts (generally 3 per day) to retrieve from Weather Underground
#define NUM_FORECASTS 3

string wunderAPI = WUND_API;
string wunderCity = WUND_CITY;
string wunderCountry = WUND_COUNTRY;

string cosmAPIKey = COSM_API_KEY;
string cosmFeed = COSM_FEED;
string cosmCPUTempDatastream = COSM_CPU_TEMP_DATASTREAM;
string cosmTempDatastream = COSM_TEMP_DATASTREAM;
string cosmPressureDatastream = COSM_PRESSURE_DATASTREAM;
string cosmHumidityDatastream = COSM_HUMIDITY_DATASTREAM;

long total_cpu_temp = 0;

int total_readings = 0;
// this is initialized to force an initial reading the first time we loop through the code
unsigned long last_reading_millis = 1000000;
unsigned long last_weather_millis = 1000000;
unsigned long last_forecast_millis = 1000000;
unsigned long last_upload_millis = 0;
// next day for which forecast will be displayed
int next_forecast = 0;

#ifdef USE_BMP085
// most recent temperature (C) returned from BMP085
float bmptemp;
// most recent pressure (Pascals) returned from BMP085
int32_t bmppress;
// most recent pressure (kPa) returned from BMP085
double bmppressK;
double total_bmp085_temp = 0;
double total_bmp085_pressure = 0;

GMBmp085 bmp;
#endif
#ifdef USE_TH02
// most recent temperature (C) returned from TH02
float th02temp;
// most recent humidity returned from TH02
float th02hum;
double total_th02_temp = 0;
double total_th02_humidity = 0;
#endif

#ifdef USE_DHT22_I2C
clsDHT22_I2C dht22;
#endif

#ifdef USE_DHT22_SERIAL
#define UART_BUFFER_LEN 128
#define END_OF_UART_MSG 10
#define UART_TIMEOUT_MS 2000
char uart_buffer[UART_BUFFER_LEN];
char dht22Temp[10];
char dht22Hum[10];
#endif

GMCosm cosm;


// bitmap for LED matrix
byte CH[] = {
	1, 8, B0000000, B0000000, B0000000, B0000000, B0000000, // space
	1, 8, B1011111, B0000000, B0000000, B0000000, B0000000, // !
	3, 8, B0000011, B0000000, B0000011, B0000000, B0000000, // "
	5, 8, B0010100, B0111110, B0010100, B0111110, B0010100, // #
	4, 8, B0100100, B1101010, B0101011, B0010010, B0000000, // $
	5, 8, B1100011, B0010011, B0001000, B1100100, B1100011, // %
	5, 8, B0110110, B1001001, B1010110, B0100000, B1010000, // &
	1, 8, B0000011, B0000000, B0000000, B0000000, B0000000, // '
	3, 8, B0011100, B0100010, B1000001, B0000000, B0000000, // (
	3, 8, B1000001, B0100010, B0011100, B0000000, B0000000, // )
	5, 8, B0101000, B0011000, B0001110, B0011000, B0101000, // *
	5, 8, B0001000, B0001000, B0111110, B0001000, B0001000, // +
	2, 8, B10110000, B1110000, B0000000, B0000000, B0000000, // ,
	4, 8, B0001000, B0001000, B0001000, B0001000, B0000000, // -
	2, 8, B1100000, B1100000, B0000000, B0000000, B0000000, // .
	4, 8, B1100000, B0011000, B0000110, B0000001, B0000000, // /
	4, 8, B0111110, B1000001, B1000001, B0111110, B0000000, // 0
	3, 8, B1000010, B1111111, B1000000, B0000000, B0000000, // 1
	4, 8, B1100010, B1010001, B1001001, B1000110, B0000000, // 2
	4, 8, B0100010, B1000001, B1001001, B0110110, B0000000, // 3
	4, 8, B0011000, B0010100, B0010010, B1111111, B0000000, // 4
	4, 8, B0100111, B1000101, B1000101, B0111001, B0000000, // 5
	4, 8, B0111110, B1001001, B1001001, B0110000, B0000000, // 6
	4, 8, B1100001, B0010001, B0001001, B0000111, B0000000, // 7
	4, 8, B0110110, B1001001, B1001001, B0110110, B0000000, // 8
	4, 8, B0000110, B1001001, B1001001, B0111110, B0000000, // 9
	1, 8, B01010000, B0000000, B0000000, B0000000, B0000000, // :
	2, 8, B10000000, B01010000, B0000000, B0000000, B0000000, // ;
	3, 8, B0010000, B0101000, B1000100, B0000000, B0000000, // <
	3, 8, B0010100, B0010100, B0010100, B0000000, B0000000, // =
	3, 8, B1000100, B0101000, B0010000, B0000000, B0000000, // >
	4, 8, B0000010, B1011001, B0001001, B0000110, B0000000, // ?
	5, 8, B0111110, B1001001, B1010101, B1011101, B0001110, // @
	4, 8, B1111110, B0010001, B0010001, B1111110, B0000000, // A
	4, 8, B1111111, B1001001, B1001001, B0110110, B0000000, // B
	4, 8, B0111110, B1000001, B1000001, B0100010, B0000000, // C
	4, 8, B1111111, B1000001, B1000001, B0111110, B0000000, // D
	4, 8, B1111111, B1001001, B1001001, B1000001, B0000000, // E
	4, 8, B1111111, B0001001, B0001001, B0000001, B0000000, // F
	4, 8, B0111110, B1000001, B1001001, B1111010, B0000000, // G
	4, 8, B1111111, B0001000, B0001000, B1111111, B0000000, // H
	3, 8, B1000001, B1111111, B1000001, B0000000, B0000000, // I
	4, 8, B0110000, B1000000, B1000001, B0111111, B0000000, // J
	4, 8, B1111111, B0001000, B0010100, B1100011, B0000000, // K
	4, 8, B1111111, B1000000, B1000000, B1000000, B0000000, // L
	5, 8, B1111111, B0000010, B0001100, B0000010, B1111111, // M
	5, 8, B1111111, B0000100, B0001000, B0010000, B1111111, // N
	4, 8, B0111110, B1000001, B1000001, B0111110, B0000000, // O
	4, 8, B1111111, B0001001, B0001001, B0000110, B0000000, // P
	4, 8, B0111110, B1000001, B1000001, B10111110, B0000000, // Q
	4, 8, B1111111, B0001001, B0001001, B1110110, B0000000, // R
	4, 8, B1000110, B1001001, B1001001, B0110010, B0000000, // S
	5, 8, B0000001, B0000001, B1111111, B0000001, B0000001, // T
	4, 8, B0111111, B1000000, B1000000, B0111111, B0000000, // U
	5, 8, B0001111, B0110000, B1000000, B0110000, B0001111, // V
	5, 8, B0111111, B1000000, B0111000, B1000000, B0111111, // W
	5, 8, B1100011, B0010100, B0001000, B0010100, B1100011, // X
	5, 8, B0000111, B0001000, B1110000, B0001000, B0000111, // Y
	4, 8, B1100001, B1010001, B1001001, B1000111, B0000000, // Z
	2, 8, B1111111, B1000001, B0000000, B0000000, B0000000, // [
	4, 8, B0000001, B0000110, B0011000, B1100000, B0000000, // backslash
	2, 8, B1000001, B1111111, B0000000, B0000000, B0000000, // ]
	3, 8, B0000010, B0000001, B0000010, B0000000, B0000000, // hat
	4, 8, B1000000, B1000000, B1000000, B1000000, B0000000, // _
	2, 8, B0000001, B0000010, B0000000, B0000000, B0000000, // `
	4, 8, B0100000, B1010100, B1010100, B1111000, B0000000, // a
	4, 8, B1111111, B1000100, B1000100, B0111000, B0000000, // b
	4, 8, B0111000, B1000100, B1000100, B0101000, B0000000, // c
	4, 8, B0111000, B1000100, B1000100, B1111111, B0000000, // d
	4, 8, B0111000, B1010100, B1010100, B0011000, B0000000, // e
	3, 8, B0000100, B1111110, B0000101, B0000000, B0000000, // f
	4, 8, B10011000, B10100100, B10100100, B01111000, B0000000, // g
	4, 8, B1111111, B0000100, B0000100, B1111000, B0000000, // h
	3, 8, B1000100, B1111101, B1000000, B0000000, B0000000, // i
	4, 8, B1000000, B10000000, B10000100, B1111101, B0000000, // j
	4, 8, B1111111, B0010000, B0101000, B1000100, B0000000, // k
	3, 8, B1000001, B1111111, B1000000, B0000000, B0000000, // l
	5, 8, B1111100, B0000100, B1111100, B0000100, B1111000, // m
	4, 8, B1111100, B0000100, B0000100, B1111000, B0000000, // n
	4, 8, B0111000, B1000100, B1000100, B0111000, B0000000, // o
	4, 8, B11111100, B0100100, B0100100, B0011000, B0000000, // p
	4, 8, B0011000, B0100100, B0100100, B11111100, B0000000, // q
	4, 8, B1111100, B0001000, B0000100, B0000100, B0000000, // r
	4, 8, B1001000, B1010100, B1010100, B0100100, B0000000, // s
	3, 8, B0000100, B0111111, B1000100, B0000000, B0000000, // t
	4, 8, B0111100, B1000000, B1000000, B1111100, B0000000, // u
	5, 8, B0011100, B0100000, B1000000, B0100000, B0011100, // v
	5, 8, B0111100, B1000000, B0111100, B1000000, B0111100, // w
	5, 8, B1000100, B0101000, B0010000, B0101000, B1000100, // x
	4, 8, B10011100, B10100000, B10100000, B1111100, B0000000, // y
	3, 8, B1100100, B1010100, B1001100, B0000000, B0000000, // z
	3, 8, B0001000, B0110110, B1000001, B0000000, B0000000, // {
	1, 8, B1111111, B0000000, B0000000, B0000000, B0000000, // |
	3, 8, B1000001, B0110110, B0001000, B0000000, B0000000, // }
	4, 8, B0001000, B0000100, B0001000, B0000100, B0000000, // ~
};
int cs = MATRIX_CS_PIN; // SPI CS pin 
int matrixCount = NUM_MATRIX;    //change this variable to set how many MAX7219's you'll use
GMMaxMatrix m(cs, matrixCount);
GMWunderground *weather;

//GMWunderground weather(WUND_API, WUND_COUNTRY, WUND_CITY);

// buffer used to copy 1 character's bitmap into LED matrix buffer - generally only requires 8 bytes
byte buffer[10];
// column index in LED matrix, including off-screen text that scrolls onto the buffer
int column = 0;

//HACK - add 2 for ending LF and null
char matrixData[MATRIX_DATA_LEN + 2];


int loop_count = 0;
DateTime dt;

int _tmain(int argc, _TCHAR* argv[])
{
	return RunArduinoSketch();
}



void setup()
{


	if (readIni("WeatherReport.ini") != 0)
	{
		Log("unable to read .ini file, bye\n");
		exit(-1);
	}

	if (!wunderAPI.empty())
	{
		weather = new GMWunderground(wunderAPI, wunderCountry, wunderCity, NUM_FORECASTS);
	}

#ifdef USE_BMP085
	if (!bmp.begin())
	{
		Log("bmp init failed bye\n");
		exit(-1);
	}
#endif

#ifdef USE_TH02
	TH02.begin();

#endif

#ifdef USE_DHT22_I2C
	if (!dht22.begin())
	{
		Log("DHT22 init failed bye\n");
		exit(-1);
	}
	//TEST CODE - REMOVE !!!!!!!!!!!!!!!!!!!!!!!!
	while (1)
	{
		delay(2000);
		float temp;
		temp = dht22.getTemperature();
		Log("DHT22 temperature = %.1f\n", temp);
	}
#endif

	if (WRITE_TO_SERIAL == 0)
	{
		m.init();
		m.setIntensity(1);
		printString("Waiting for data...");
	}
	if (WRITE_TO_SERIAL >= 1)
	{
		Serial.begin(CBR_9600, Serial.SERIAL_8N1);
		Serial.print("D0:Hello world\n");
	}
	if (WRITE_TO_SERIAL == 2)
	{
		Serial1.begin(CBR_9600, Serial.SERIAL_8N1);
		//Serial1.begin(CBR_9600, Serial.SERIAL_8N1);
		Serial1.print("D0:Hello display 2\n");

	}

}

// the loop routine runs over and over again forever:
void loop()
{
	int result;


#ifdef USE_DHT22_SERIAL
	getTempAndHumidityFromSerial();
#endif
	// HACK - reset the matrix after every 100 loops - in case 7219s get confused

	if (loop_count >= MATRIX_RESET_FREQUENCY)
	{
		if (WRITE_TO_SERIAL == 0)
		{
			m.init();
			m.setIntensity(1);
		}
		loop_count = 0;
	}
	else
	{
		loop_count += 1;
	}

	double avgValue;
	int offset;

	// read local sensors
	if (millis() - last_reading_millis >= MEASUREMENT_INTERVAL
		// check if millis has looped back to 0
		|| (millis() < last_reading_millis))
	{
		total_readings += 1;
		// not supported on Galileo2 yet
		//total_cpu_temp += readCPUTemp();
#ifdef USE_BMP085
		bmptemp = bmp.readTemperature();
		total_bmp085_temp += bmptemp;
		bmppress = bmp.readPressure();
		bmppressK = bmppress / (double)1000;
		total_bmp085_pressure += bmppressK;
		Log("bmp085 temp = %.2f press = %.1f\r\n", bmptemp, bmppressK);
#endif


#ifdef USE_TH02
		th02temp = TH02.ReadTemperature();
		total_th02_temp += th02temp;
		th02hum = TH02.ReadHumidity();
		total_th02_humidity += th02hum;
		Log("TH02 temp = %.1fC humidity = %.1f%%\r\n", th02temp, th02hum);
#endif

		last_reading_millis = millis();
	}



	// read online APIs
	if (weather != nullptr && (millis() - last_weather_millis >= WEATHER_MEASUREMENT_INTERVAL
		// check if millis has looped back to 0
		|| (millis() < last_weather_millis)))
	{
		result = weather->GetCurrent();
		Log("weather->GetCurrent returned %d\r\n", result);
		if (result == 0)
		{
			last_weather_millis = millis();
		}
	}

	if (weather != nullptr && (millis() - last_forecast_millis >= FORECAST_MEASUREMENT_INTERVAL
		// check if millis has looped back to 0
		|| (millis() < last_forecast_millis)))
	{
		result = weather->GetForecast();
		Log("weather->GetForecast returned %d\r\n", result);
		if (result == 0)
		{
			last_forecast_millis = millis();
		}
	}


	// fill the data buffer with 00s
	//memset(matrixData, 0, sizeof(matrixData));

	// put date and time in buffer
	offset = 0;
	column = 0;

	if (WRITE_TO_SERIAL >= 1)
	{
		// add prefix expected by Arduino code
		offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "D0:");
	}

	// write time to LED matrix
	offset += dt.GetTime(matrixData + offset, MATRIX_DATA_LEN - offset, false, false);
	// add space
	offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, " ");

	// write date to LED matrix --> uncomment if you want the date to be displayed
	//offset += dt.GetDate(matrixData + offset, MATRIX_DATA_LEN - offset);
	// add space
	//offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, " ");

#ifdef USE_BMP085
	// write temperature to LED matrix
	offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%.1fC ", bmptemp);
#endif
#ifdef USE_TH02
	offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%.1fC %.1f%% ", th02temp, th02hum);
#endif
#ifdef USE_DHT22_SERIAL
	offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%sC %s%% ", dht22Temp, dht22Hum);
#endif
	// add space
	//offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, " ");
	// write indoor barometer to LED matrix
	//offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%.1fkPa", bmppressK);
	// add space
	//offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, " ");

	// write current weather from Weather Underground API
	//TODO - check weather->lastreading time and skip if out-of-date
	if (weather != nullptr)
	{

		offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "Out %dC ", weather->TempC);
		if (weather->TempC != weather->TempFeelsLikeC)
		{
			offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "Feels Like %dC ", weather->TempFeelsLikeC);
		}
		if (weather->WindKPH > 5)
		{
			offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "Wind %d kph", weather->WindKPH);
		}
	}
	if (WRITE_TO_SERIAL >= 1)
	{
		Log("Writing to serial %s\n", matrixData);
		//HACK - seems that Serial.println isn't sending a CR, causing Arduino to hang
		matrixData[offset] = '\n';
		matrixData[offset + 1] = 0x00;
		Serial.print(matrixData);
	}
	else
	{

		m.clear();
		printString(matrixData);
	}



	// NOTE: the 2nd part of this if statement  -- millis < last_upload_millis -- checks if the millisecond 
	// counter has looped around to 0 (which occurs every 70 days)
	if (!(cosmAPIKey.empty() || cosmFeed.empty()))
	{
		if (millis() - last_upload_millis >= COSM_UPLOAD_INTERVAL || (millis() < last_upload_millis))
		{
			if (total_readings > 0)
			{

				// not supported on Galileo 2 yet
				//avgValue = total_cpu_temp / (double)total_readings;


				//cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmCPUTempDatastream.c_str(), avgValue);
#ifdef USE_BMP085
				avgValue = total_bmp085_temp / (double)total_readings;
				cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmTempDatastream.c_str(), avgValue);

				avgValue = total_bmp085_pressure / (double)total_readings;
				cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmPressureDatastream.c_str(), avgValue);
				//TEST - record outside pressure for comparison
				cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), "PressureOutdoors", weather->PressureMB);
				total_bmp085_pressure = 0;
				total_bmp085_temp = 0;
#endif
#ifdef USE_TH02
				avgValue = total_th02_temp / (double)total_readings;
				cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmTempDatastream.c_str(), avgValue);

				avgValue = total_th02_humidity / (double)total_readings;
				cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmHumidityDatastream.c_str(), avgValue);
				total_th02_humidity = 0;
				total_th02_temp = 0;
#endif
#ifdef USE_DHT22_SERIAL
				if (strlen(dht22Temp) > 0)
				{
					cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmTempDatastream.c_str(), stof(dht22Temp));
				}

				if (strlen(dht22Hum) > 0)
				{
					cosm.SendToCosm(cosmAPIKey.c_str(), stoi(cosmFeed), cosmHumidityDatastream.c_str(), stof(dht22Hum));
				}
#endif
				total_readings = 0;
				total_cpu_temp = 0;
				last_upload_millis = millis();
			}
		}
	}
	if (WRITE_TO_SERIAL == 1)
	{
		// wait 20 seconds for the Arduino to do its thing
		getTempAndHumidityFromSerial();
		delay(20000);
		getTempAndHumidityFromSerial();
	}
	else if (WRITE_TO_SERIAL == 0)
	{
		// wait 1 seconds before scrolling
		delay(1000);
		// scroll until the last column is visible 
		column = column - (8 * matrixCount);
		for (int i = 0; i < column; i++)
		{
			m.shiftLeft(false, false);
			delay(MATRIX_SCROLL_MS);
		}

		// delay before clearing the display
		delay(2000);
	}


	if (WRITE_TO_SERIAL == 2)
	{
		// write the next forecast to the 2nd display
		if (weather != nullptr)
		{
			if (!weather->ForecastDays[next_forecast].empty())
			{
				offset = sprintf_s(matrixData, MATRIX_DATA_LEN, "D0:Forecast %s - ", weather->ForecastDays[next_forecast].c_str());
				// forecast can be wordy, so truncate it if necessary
				offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%s", weather->Forecasts[next_forecast].substr(0, MATRIX_DATA_LEN - offset - 2).c_str());

				Log("Writing to serial1 %s\n", matrixData);
				//HACK - seems that Serial.println isn't sending a CR, causing Arduino to hang
				matrixData[offset] = '\n';
				matrixData[offset + 1] = 0x00;
				Serial1.print(matrixData);

				// advance to next day - its forecast will be displayed the next time through
				next_forecast += 1;
				if (next_forecast >= weather->numDaysForecast)
				{
					next_forecast = 0;
				}

				// give displays time to scroll before moving on
				delay(15000);


			}
		}
	}
	else
	{
		// every 5th time through the loop, display the forecast
		if (loop_count % 20 == 1 && weather != nullptr)
		{
			for (int day = 0; day < 3; day++)
			{

				if (!weather->ForecastDays[day].empty())
				{
					offset = sprintf_s(matrixData, MATRIX_DATA_LEN, "D0:Forecast %s - ", weather->ForecastDays[day].c_str());
					// forecast can be wordy, so truncate it if necessary
					offset += sprintf_s(matrixData + offset, MATRIX_DATA_LEN - offset, "%s", weather->Forecasts[day].substr(0, MATRIX_DATA_LEN - offset - 2).c_str());
					if (WRITE_TO_SERIAL == 1)
					{
						Log("Writing to serial %s\n", matrixData);
						//HACK - seems that Serial.println isn't sending a CR, causing Arduino to hang
						matrixData[offset] = '\n';
						matrixData[offset + 1] = 0x00;
						Serial.print(matrixData);
						int delayTime = offset / 4;
						// give Arduino time to read data before moving onto next day
						//TODO - have Arduino send feedback when it has scrolled through all of the text
						getTempAndHumidityFromSerial();
						delay(delayTime * 1000);
						
					}
					//TODO
					//else
					//{

					//	m.clear();
					//	printString(matrixData);
					//}
				}
			}

		}
	}
}

void printString(char* s)
{
	// int col = 0;
	while (*s != 0)
	{
		if (*s < 32) continue;
		char c = *s - 32;
		memcpy(buffer, CH + 7 * c, 7);
		m.writeSprite(column, 0, buffer);
		m.setColumn(column + buffer[0], 0);
		column += buffer[0] + 1;
		s++;
	}
}

int readCPUTemp()
{
	//float temperatureInDegreesCelcius = 1.0f;	// Storage for the temperature value

	// reads the analog value from this pin (values range from 0-1023)
	int temperatureInDegreesCelcius = analogRead(-1);

	Log(L"Temperature: %d Celcius\n", temperatureInDegreesCelcius);
	return temperatureInDegreesCelcius;
}

int readIni(char * filename)
{
	string input_str, ini_setting, ini_value;
	ifstream file_in(filename);
	int pos;
	if (!file_in) {
		Log("Ini file not found %s\r\n", filename);
		return 1;
	}
	while (!file_in.eof()) {

		getline(file_in, input_str);
		input_str = delSpaces(input_str);
		// comments start with certain characters - ignore
		if (input_str.find_first_of(";/'") == 0)
		{
			continue;
		}
		pos = input_str.find("=");
		if (pos > 0 && ((pos + 1) < input_str.length()))
		{
			ini_setting = input_str.substr(0, pos);
			ini_value = input_str.substr(pos + 1);
			if (ini_setting == "COSM_API_KEY")
			{
				cosmAPIKey = ini_value;
			}
			else if (ini_setting == "COSM_FEED_ID")
			{
				cosmFeed = ini_value;
			}
			else if (ini_setting == "COSM_CPU_TEMP_DATASTREAM")
			{
				cosmCPUTempDatastream = ini_value;
			}
			else if (ini_setting == "COSM_TEMP_DATASTREAM")
			{
				cosmTempDatastream = ini_value;
			}
			else if (ini_setting == "COSM_PRESSURE_DATASTREAM")
			{
				cosmPressureDatastream = ini_value;
			}
			else if (ini_setting == "COSM_HUMIDITY_DATASTREAM")
			{
				cosmHumidityDatastream = ini_value;
			}
			else if (ini_setting == "WUNDERGROUND_API")
			{
				wunderAPI = ini_value;
			}
			else if (ini_setting == "WUNDERGROUND_COUNTRY")
			{
				wunderCountry = ini_value;
			}
			else if (ini_setting == "WUNDERGROUND_CITY")
			{
				wunderCity = ini_value;
			}
			else
			{
				Log("ERROR - Invalid .ini setting %s\n", ini_setting.c_str());
			}
		}

	}
	file_in.close();

	// display the values
	Log("API Key = %s\r\n", cosmAPIKey.c_str());
	Log("Feed ID = %s\r\n", cosmFeed.c_str());
	Log("CPU Temp Datastream = %s\r\n", cosmCPUTempDatastream.c_str());
	Log("Temp Datastream = %s\r\n", cosmCPUTempDatastream.c_str());
	Log("Pressure Datastream = %s\r\n", cosmCPUTempDatastream.c_str());
	return 0;
}

// STL function to  remove all spaces from a string
// take from StackOverflow answer: http://stackoverflow.com/questions/83439/remove-spaces-from-stdstring-in-c
string delSpaces(string &str)
{
	str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
	return str;
}





void getTempAndHumidityFromSerial()
{
#ifdef USE_DHT22_SERIAL
	while (Serial.available())
	{
		if (fillBuffer() > 0) // fillBuffer returns 0 when nothing is read or the data looks invalid
		{
			// commands start with "x#:" 
			if (uart_buffer[1] == ':')
			{

				// D#: - display the string via buffer #
				if (uart_buffer[0] == 'T')
				{
					copyToBuffer(dht22Temp, 10);
					Log("Got temperature %s\n", dht22Temp);
					total_readings = 1; // data is ready to send to Cosm
				}
				else if (uart_buffer[0] == 'H')
				{
					copyToBuffer(dht22Hum, 10);
					Log("Got humidity %s\n", dht22Hum);
					total_readings = 1; // data is ready to send to Cosm
				}
				else
				{
					Log("Invalid buffer data type: %s\n", uart_buffer);
				}


			}
			else
			{
				Log("Invalid buffer format: %s\n", uart_buffer);
			}

		}
	}
#endif

}

#ifdef USE_DHT22_SERIAL
// Read next command sent to serial UART
// - Note - this is old code intended for use with XBee at 9600 baud, so it could be rewritten for faster speed
int fillBuffer()
{
	char charRead = ' ';
	int i = 0;
	unsigned long lngStartTime = millis();
#if DEBUG
	Serial.println("in fillbuffer");
#endif	

	// wait up to 5 seconds for the full message
	while (charRead != END_OF_UART_MSG && charRead != 0x00 && (millis() - lngStartTime < UART_TIMEOUT_MS) && i < UART_BUFFER_LEN)
	{
		if (Serial.available())
		{
			charRead = (char)Serial.read();
			if (charRead != 13) // ignore CR
			{
				uart_buffer[i++] = charRead;
			}

			if (!Serial.available())
			{
				delay(200); // give sender time to catch up
			}
		}
	}
	if (charRead == END_OF_UART_MSG && i > 2 && uart_buffer[1] == ':')
	{
		// replace END_OF_BUFFER character with a string-ending null
		uart_buffer[i - 1] = 0x00;

		Log("got data from UART: %s\n", uart_buffer);
		return i;
	}
	else
	{
		// too much data, or too little data
		Log("Invalid data read from UART\n");
		if (i > 0)
		{
			uart_buffer[i - 1] = 0x00;
			Log("%s\n", uart_buffer);
		}
		return 0;
	}

}

// Copy from uartbuffer to correct display buffer or status lines
void copyToBuffer(char *target, int intMaxLen)
{
	int i = 0;
	int j = 2; // begin with 4th byte of array
	do {
		target[i++] = uart_buffer[j++];
	} while (i < intMaxLen && uart_buffer[j] != 0x00);

	// include ending null
	// if too large for buffer, then truncate the string
	if (i == intMaxLen)
	{
		target[i - 1] = 0x00;
	}
	else
	{
		target[i] = 0x00;
	}
}
#endif