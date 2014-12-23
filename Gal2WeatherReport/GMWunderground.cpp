#include "GMWunderground.h"


GMWunderground::GMWunderground(std::string pAPIkey, std::string pcountry, std::string pcity, int pnumDaysForecast)
{
	APIkey = pAPIkey;
	country = pcountry;
	city = pcity;
	numDaysForecast = pnumDaysForecast;
	ForecastDays = new std::string[numDaysForecast];
	Forecasts = new std::string[numDaysForecast];
}

int GMWunderground::GetCurrent()
{

	DateTime dt;

	int iResult;
	//int64_t lResult;
	std::string jsonErrorMsg;
	char *jsonData;

	if (callAPI("conditions") != 0)
	{
		return -1;
	}
		
	//TODO - this code assumes we get the full output string in one recv
	jsonData = strchr(recvbuf, '{');
	if (jsonData == NULL)
	{
		writeToLog("GMWunderground.GetCurrent: API returned no JSON data.");
		return -2;
	}
	if (reader.parse(jsonData, root))
	{
		const Json::Value resp = root["response"];
		const Json::Value features = resp["features"];
		iResult = features.get("conditions", 0).asInt();
		if (iResult != 1)
		{
			Log("GMWunderground.GetCurrent: API returned %d conditions\r\n", iResult);
			return -2;
		}

		const Json::Value obs = root["current_observation"];
		Json::Value::Members obs_values = obs.getMemberNames();
		for (uint16_t i = 0; i < obs_values.size(); i++)
		{
			std::string memberName = obs_values[i];
			Json::Value memberVal = obs[memberName];
			Log("name %s value %s\r\n", memberName.c_str(), memberVal.toStyledString().c_str());
		}

		Json::Value currentobs = root["current_observation"];
		iResult = getIntFromJson(currentobs, "temp_c", &TempC);
		// if the temperature wasn't in the results, then something went wrong, so don't bother trying to set the other variables
		if (iResult == 0)
		{
			iResult = getIntFromJson(currentobs, "temp_f", &TempF);
			iResult = getStringFromJson(currentobs, "feelslike_c", &TempFeelsLikeC);
			iResult = getStringFromJson(currentobs, "feelslike_f", &TempFeelsLikeF);
			iResult = getStringFromJson(currentobs, "wind_kph", &WindKPH);
			iResult = getStringFromJson(currentobs, "wind_mph", &WindMPH);
			iResult = getStringFromJson(currentobs, "pressure_mb", &PressureMB);
			iResult = getStringFromJson(currentobs, "observation_epoch", &strTimeLastObs);
		}

		//iResult = root["current_observation"].get("temp_c", -99).asInt();
		//if (iResult == -99)
		//{
		//	Log("GMWunderground.GetCurrent: temp_c not found\r\n");
		//	return -3;
		//}
		//TempC = iResult;
		//strTimeLastObs = root["current_observation"].get("observation_epoch", 0).toStyledString();
		//lResult = _atoi64(root["current_observation"].get("observation_epoch", 0).toStyledString().c_str());
		//dt.UnixTimeToSystemTime(lResult, &timeLastObs);
	}
	else
	{
		jsonErrorMsg = reader.getFormattedErrorMessages();
		writeToLog("GMWunderground.GetCurrent: JSON parse failed: %s\r\n", jsonErrorMsg.c_str());
		return -1;
	}

		//Log(recvbuf);
	


	//} while (bytesRead > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

int GMWunderground::GetForecast()
{

	DateTime dt;

	int iResult;
	//int64_t lResult;
	std::string jsonErrorMsg;
	char *jsonData;

	if (callAPI("forecast") != 0)
	{
		return -1;
	}

	//TODO - this code assumes we get the full output string in one recv
	jsonData = strchr(recvbuf, '{');
	if (jsonData == NULL)
	{
		writeToLog("GMWunderground.GetForecast: API returned no JSON data.");
		return -2;
	}
	if (reader.parse(jsonData, root))
	{
		const Json::Value resp = root["response"];
		const Json::Value features = resp["features"];
		iResult = features.get("forecast", 0).asInt();
		if (iResult != 1)
		{
			writeToLog("GMWunderground.GetForecast: API returned %d forecasts\r\n", iResult);
			return -2;
		}

		const Json::Value forecast = root["forecast"];
		//getMembersFromJson(forecast);
		const Json::Value forecast2 = forecast["txt_forecast"];
		//getMembersFromJson(forecast2);
		const Json::Value forecastdayArray = forecast2["forecastday"];
		int intNumDays = forecastdayArray.size();

		// NOTE - skip the 1st day's forecast - it's usually already past
		if (intNumDays > numDaysForecast + 1)
		{
			intNumDays = numDaysForecast + 1;
		}

		for (int day = 1; day < intNumDays; day++)
		{
			const Json::Value dayForecast = forecastdayArray[day];
			std::string dayname;
			std::string daytext;
			getStringFromJson(dayForecast, "title", &dayname);
			getStringFromJson(dayForecast, "fcttext_metric", &daytext);
			ForecastDays[day - 1] = dayname;
			//cleanString(daytext);
			Forecasts[day - 1] = daytext;
		}


	}
	else
	{
		jsonErrorMsg = reader.getFormattedErrorMessages();
		writeToLog("GMWunderground.GetForecast: JSON parse failed: %s\r\n", jsonErrorMsg.c_str());
		return -1;
	}


	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}


int GMWunderground::callAPI(char *APIName)
{

	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;


	int iResult;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		writeToLog("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("api.wunderground.com", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		writeToLog("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			writeToLog("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			writeToLog("connect failed with socket error");
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		writeToLog("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// GET string sent to Wunderground, with embedded format strings for the API key, country and city
	const char *GET_FROM_WUNDERGROUND = "GET /api/%s/%s/q/%s/%s.json HTTP/1.1\r\nHost: \
												api.wunderground.com\r\nX-Target-URI: http://api.wunderground.com\r\nConnection: \
														Keep-Alive\r\n\r\n";
	// NOTE - subtract 2 from the data buffer length - it can't include the cr/lf
	sendbuflen = sprintf_s(sendbuf, DEFAULT_BUFLEN, GET_FROM_WUNDERGROUND, APIkey.c_str(), APIName, country.c_str(), city.c_str());

	// Send the HTML
	//iResult = send(ConnectSocket, sendbuf, sendbuflen, 0);
	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		writeToLog("html send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}


	// shutdown the connection since no more data will be sent
	//iResult = shutdown(ConnectSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) {
	//	Log("shutdown failed with error: %d\n", WSAGetLastError());
	//	closesocket(ConnectSocket);
	//	WSACleanup();
	//	return 1;
	//}
	int bytesRead;

	// DAW - 10/25/14 - removed while loop - 2nd recv hung
	// Receive until the peer closes the connection
	///do {

	bytesRead = recv(ConnectSocket, recvbuf, RECV_BUFLEN, 0);
	if (bytesRead > 0)
	{

		Log("Bytes received: %d\n", bytesRead);

		recvbuf[bytesRead] = 0x00;

		//TODO - check for HTML error code
		return 0;
	}
	else
	{
		return 2;
	}
	
}

int GMWunderground::getIntFromJson(Json::Value jsonvalue, const char *key, int *result)
{
	int ret;
	ret = jsonvalue.get(key, -999).asInt();
	if (ret == -999)
	{
		writeToLog("GMWunderground.getInFromJson %s not found\r\n", key);
		
		return -1;
	}
	else
	{
		*result = ret;
		return 0;
	}
}

int GMWunderground::getStringFromJson(Json::Value jsonvalue, const char *key, std::string *result)
{
	std::string ret;
	ret = jsonvalue.get(key, -999).asString();
	if (ret == "-999")
	{
		writeToLog("GMWunderground.getStringFromJson %s not found\r\n", key);

		return -1;
	}
	else
	{
		*result = ret;
		return 0;
	}
}

int GMWunderground::getStringFromJson(Json::Value jsonvalue, const char *key, int *result)
{
	std::string ret;
	ret = jsonvalue.get(key, -999).asString();
	if (ret == "-999")
	{
		writeToLog("GMWunderground.getStringFromJson %s not found\r\n", key);

		return -1;
	}
	else
	{
		*result = std::stoi(ret);
		return 0;
	}
}

void GMWunderground::getMembersFromJson(Json::Value obs)
{
	//const Json::Value obs = root[key];
	//Log("getting JSON members for %s\n", key);
	Json::Value::Members obs_values = obs.getMemberNames();
	for (uint16_t i = 0; i < obs_values.size(); i++)
	{
		std::string memberName = obs_values[i];
		Json::Value memberVal = obs[memberName];
		Log("name %s value %s\n", memberName.c_str(), memberVal.toStyledString().c_str());
	}
}

void GMWunderground::writeToLog(const char *format, ...)
{
	// NOTE - the following is copied from the Log function in arduino.h
	va_list args;
	int len = 0;
	char *buffer = NULL;

	va_start(args, format);
	len = _vscprintf(format, args) + 1;
	buffer = new char[len];
	if (buffer != NULL)
	{
		len = vsprintf_s(buffer, len, format, args);
		printf(buffer);

		std::fstream file_io("BMP085MatrixLog.txt");
		if (!file_io) {
			file_io.open("BMP085MatrixLog.txt", std::ios_base::out);
			file_io.close();
			file_io.open("BMP085MatrixLog.txt");
		}
		file_io << buffer << std::endl;
		file_io.close();
	}
}

// Based on http://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
//void GMWunderground::cleanString(std::string str)
//{
//	// remove LFs and nulls that are sometimes embedded in Wunderground's forecast strings
//	str.erase(std::remove(str.begin(), str.end(), '0x00'), str.end());
//	str.erase(std::remove(str.begin(), str.end(), '0x00'), str.end());
//}


GMWunderground::~GMWunderground()
{
}
