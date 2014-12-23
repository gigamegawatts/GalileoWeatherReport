#include "GMCosm.h"


GMCosm::GMCosm()
{
}

int GMCosm::SendToCosm(const char * APIkey, int feedID, const char * datastreamID, double dataValue)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	//TODO - catch other uninitialized values
	if (strlen(APIkey) == 0 || strlen(datastreamID) == 0)
	{
		// don't even bother trying
		Log("Cosm API key and datastream ID are required");
		return -99;
	}

	// PUT string sent to COSM, with embedded format strings for the feed ID, API key, and length of data in databuf 
	// - this string is intended for use with sprintf, storing the actual value to be sent to Cosm in sendbuf
	const char * PUT_TO_COSM = "PUT /v2/feeds/%d.csv HTTP/1.1\r\nHost: api.cosm.com\r\nX-ApiKey: %s\r\nUser-Agent: WinGalileo\r\nContent-Length: %d\r\nContent-Type: text/csv\r\nConnection: close\r\n\r\n";
	//char *sendbuf = "PUT /v2/feeds/18751.csv HTTP/1.1\r\nHost: api.cosm.com\r\nX-ApiKey: QJXUKIO0-WIX8pl1gjPDH2oItvPaS5gzTRxvTDsoMIc\r\nUser-Agent: WinGalileo\r\nContent-Length: 18\r\nContent-Type: text/csv\r\nConnection: close\r\n\r\n";
	//char *databuf = "GalileoTest,123.45\r\n";

	char sendbuftest[DEFAULT_BUFLEN] = "PUT /v2/feeds/37081.csv HTTP/1.1\r\nHost: api.cosm.com\r\nX-ApiKey: QJXUKIO0-WIX8pl1gjPDH2oItvPaS5gzTRxvTDsoMIc\r\nUser-Agent: WinGalileo\r\nContent-Length: 19\r\nContent-Type: text/csv\r\nConnection: close\r\n\r\n";
	//                    "PUT /v2/feeds/37081.csv HTTP/1.1\r\nHost: api.cosm.com\r\nX-ApiKey: QJXUKIO0-WIX8pl1gjPDH2oItvPaS5gzTRxvTDsoMIc\r\nUser-Agent: WinGalileo\r\nContent-Length: 19\r\nContent-Type: text/csv\r\nConnection: close\r\n\r\n"
	char databuftest[50] = "OfficeGalTemp,39.00\r\n";
	//					"OfficeGalTemp,38.00\r\n"

	int iResult;

	//// Validate the parameters
	//if (argc != 2) {
	//	printf("usage: %s server-name\n", argv[0]);
	//	return 1;
	//}
	Log("Sending to Cosm, feed %d, stream %s, value %.2f \n", feedID, datastreamID, dataValue);

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		Log("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("www.cosm.com", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		Log("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			Log("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			Log("connect failed with socket error");
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		Log("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// NOTE!!! - forces data to have 2 decimal points - change format string if necessary
	databuflen = sprintf_s(databuf, 50, "%s,%.2f\r\n", datastreamID, dataValue);

	// NOTE - subtract 2 from the data buffer length - it can't include the cr/lf
	sendbuflen = sprintf_s(sendbuf, DEFAULT_BUFLEN, PUT_TO_COSM, feedID, APIkey, strlen(databuf) - 2);

	// Send the HTML
	//iResult = send(ConnectSocket, sendbuf, sendbuflen, 0);
	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		Log("html send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// send the data
	iResult = send(ConnectSocket, databuf, strlen(databuf), 0);
	if (iResult == SOCKET_ERROR) {
		Log("data send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	//printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		Log("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// Receive until the peer closes the connection
	do {

		iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
		if (iResult > 0)
		{

			Log("Bytes received: %d\n", iResult);

			recvbuf[iResult - 1] = 0x00;
			Log(recvbuf);
		}
		else if (iResult == 0)
			Log("Connection closed\n");
		else
			Log("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}

