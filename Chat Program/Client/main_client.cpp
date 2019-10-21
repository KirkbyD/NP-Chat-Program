#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>

#include <cProtocol.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 1
#define DEFAULT_PORT "5150"

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	// #1 socket
	SOCKET connectSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() successful!\n");
	}

	// #2 connect
	// Attempt to connect to the server until a socket succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to the server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket() failed with error code %d\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Attempt to connect to the server
		iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("connect() failed with error code %d\n", WSAGetLastError());
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to the server!\n");
		WSACleanup();
		return 1;
	}
	printf("Successfully connected to the server on socket %d!\n", (int)connectSocket);

	// #3 write & read
	Protocol prot;
	//prot.UserSendMessage("RoomName", "Hello");
	//prot.UserRecieveMessage("Someone","RoomName", "Hello");
	//prot.UserLeaveRoom("RoomName");
	prot.UserJoinRoom("RoomName");
	std::vector<uint8_t> vect = prot.GetBuffer();

	printf("Sending a packet to the server...\n");

	iResult = send(connectSocket, (char*)vect.data(), (int)vect.size(), 0);
	if (iResult == SOCKET_ERROR)
	{
		printf("send() failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	printf("Bytes sent: %d\n", iResult);

	/*// Receive a message from the server before quitting
	char header[DEFAULT_BUFLEN];
	int header_size = DEFAULT_BUFLEN;
	printf("Waiting to receive data from the server...\n");
	system("Pause");
	iResult = recv(connectSocket, header, header_size, 0);

	if (iResult > 0)
	{
		printf("%s\n", header);
		printf("Bytes received: %d\n", iResult);
	}
	else if (iResult == 0)
	{
		printf("Connection closed\n");
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	std::vector<uint8_t> vect2;
	for (int i = 0; i < (int)header[0]; i++)
	{
		vect2.push_back('0');
	}

	iResult = recv(connectSocket, (char*)vect.data(), (int)vect.size(), 0);

	if (iResult > 0)
	{
		printf("%s\n", vect.data());
		printf("Bytes received: %d\n", iResult);
	}
	else if (iResult == 0)
	{
		printf("Connection closed\n");
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}*/

	// #4 close
	closesocket(connectSocket);
	WSACleanup();

	system("Pause");

	return 0;
}