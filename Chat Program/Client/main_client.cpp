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

	DWORD NonBlock = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to the server!\n");
		WSACleanup();
		return 1;
	}
	printf("Successfully connected to the server on socket %d!\n", (int)connectSocket);

	FD_SET ReadSet;
	int is_Active;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 0;
		// Initialize our read set
		FD_ZERO(&ReadSet);
		FD_SET(connectSocket, &ReadSet);

		// Call our select function to find the sockets that
		// require our attention
		//printf("Waiting for select()...\n");
		is_Active = select(0, &ReadSet, NULL, NULL, &tv);
		if (is_Active == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			//printf("select() is successful!\n");
		}

		//Replace with user input
		/**********************************************************/
		// #3.1 write
		Protocol prot;
		//prot.UserSendMessage("RoomName", "Hello");
		//prot.UserLeaveRoom("RoomName");
		prot.UserJoinRoom("RoomName");
		std::vector<uint8_t> sendVect = prot.GetBuffer();

		printf("Sending a packet to the server...\n");

		iResult = send(connectSocket, (char*)sendVect.data(), (int)sendVect.size(), 0);
		if (iResult == SOCKET_ERROR)
		{
			printf("send() failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}
		printf("Bytes sent: %d\n", iResult);
		/**************************************************************/


		if (FD_ISSET(connectSocket, &ReadSet))
		{
			is_Active--;

			// #3.2 read

			/*// Receive a message from the server before quitting
			printf("Waiting to receive data from the server...\n");
			system("Pause");*/

			std::vector<uint8_t> recvVect1;
			for (int i = 0; i < INT_SIZE * 2; i++)
			{
				recvVect1.push_back('0');
			}

			iResult = recv(connectSocket, (char*)recvVect1.data(), INT_SIZE * 2, 0);

			if (iResult < 0)
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}

			Buffer buf;
			buf.ReceiveBufferContent(recvVect1);
			printf("Packet Length: %i\n", buf.readInt32LE(INT_SIZE * 0));
			printf("Message ID: %i\n", buf.readInt32LE(INT_SIZE * 1));

			std::vector<uint8_t> recvVect2;
			for (int i = 0; i < buf.readInt32LE(0) - INT_SIZE * 2; i++)
			{
				recvVect2.push_back('0');
			}

			iResult = recv(connectSocket, (char*)recvVect2.data(), (int)recvVect2.size(), 0);

			if (iResult < 0)
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}

			buf.ReceiveBufferContent(INT_SIZE * 2, recvVect2);

			if (buf.readInt32LE(INT_SIZE) == 3)
			{
				//Recieve
				int packet_length = buf.readInt32LE(INT_SIZE * 0);
				int message_id = buf.readInt32LE(INT_SIZE * 1);
				int name_length = buf.readInt32LE(INT_SIZE * 2);
				std::string name = buf.ReadString(INT_SIZE * 3, name_length);
				int room_name_length = buf.readInt32LE(INT_SIZE * 3 + name_length);
				std::string room_name = buf.ReadString(INT_SIZE * 4 + name_length, room_name_length);
				int message_length = buf.readInt32LE(INT_SIZE * 4 + name_length + room_name_length);
				std::string message = buf.ReadString(INT_SIZE * 5 + name_length + room_name_length, message_length);

				printf("[%s] %s: %s\n", room_name.c_str(), name.c_str(), message.c_str());
			}
		}
	}

	// #4 close
	closesocket(connectSocket);
	WSACleanup();

	system("Pause");

	return 0;
}