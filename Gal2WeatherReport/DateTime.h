#pragma once
#include "arduino.h"
#include <string.h>

class DateTime
{
private:
	SYSTEMTIME lt;
	struct tm timeinfo;
	//String months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	void getCurrentTime();
public:
	DateTime();
	~DateTime();
	int GetShortDate(char *buffer, int bufferlen);
	int GetDate(char *buffer, int bufferlen);
	int GetTime(char *buffer, int bufferlen, bool bln24Hr, bool blnAMPM);
	void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst);
};

