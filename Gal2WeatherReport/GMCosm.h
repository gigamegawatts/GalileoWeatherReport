#pragma once
#define _WINSOCKAPI_  // DAW - on Gal2, prevent inclusion of winsock.h
#include "arduino.h"
#define WIN32_LEAN_AND_MEAN


#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")



#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "80" 


class GMCosm
{
public:
	GMCosm();
	int SendToCosm(const char * apiKey, int feedID, const char * datastreamID, double dataValue);

private:

	// sendbuf holds the HTML PUT request including headers
	char sendbuf[DEFAULT_BUFLEN];
	// databuf holds the data part of the PUT request send to Cosm: it consists of <datastreamname>,<datavalue>
	char databuf[50];
	// recvbuf holds whatever response is returned by Cosm - not currently used
	char recvbuf[DEFAULT_BUFLEN];
	// lengths of actual strings stored in sendbuf, databuf
	int databuflen, sendbuflen;

};

