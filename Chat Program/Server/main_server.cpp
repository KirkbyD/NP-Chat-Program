#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <map>

#include <cProtocol.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5150"

// Client structure
struct ClientInfo {
	SOCKET socket;

	// Buffer information (this is basically you buffer class)
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

	// We also need to cleanup the ClientInfo data
	// TODO: Delete Client
}

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;

	//Multirooming
	std::map<std::string, std::vector<ClientInfo*>> m_Rooms;

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

	// #1 Socket
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	// Create a SOCKET for connecting to the server
	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	// #2 Bind - Setup the TCP listening socket
	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	// We don't need this anymore
	freeaddrinfo(addrResult);

	// #3 Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}

	// Change the socket mode on the listening socket from blocking to
	// non-blocking so the application will not block waiting for requests
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	DWORD flags;
	DWORD RecvBytes;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 1;
		// Initialize our read set
		FD_ZERO(&ReadSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &ReadSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		// Call our select function to find the sockets that
		// require our attention
		//printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			//printf("select() is successful!\n");
		}

		// #4 Check for arriving connections on the listening socket
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");

					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					//info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					printf("New client connected on socket %d\n", (int)acceptSocket);
				}
			}
		}

		// #5 recv & send
		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;

				std::vector<uint8_t> vect;
				for (int i = 0; i < 8; i++)
				{
					vect.push_back('0');
				}
				int iRecvResult = recv(client->socket, (char*)vect.data(), 8, 0);

				if (iRecvResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					if (iRecvResult == 0)
					{
						RemoveClient(i);
					}
					else
					{
						Buffer buf;
						buf.ReceiveBufferContent(vect);
						printf("Size of message: %i\n", buf.readInt32LE(INT_SIZE * 0));
						printf("MESSAGE ID: %i\n", buf.readInt32LE(INT_SIZE * 1));

						std::vector<uint8_t> vect2;
						for (int i = 0; i < buf.readInt32LE(0) - INT_SIZE * 2; i++)
						{
							vect2.push_back('0');
						}

						int iRecvResult = recv(client->socket, (char*)vect2.data(), (int)vect2.size(), 0);

						if (iRecvResult == SOCKET_ERROR)
						{
							if (WSAGetLastError() == WSAEWOULDBLOCK)
							{
								// We can ignore this, it isn't an actual error.
							}
							else
							{
								printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
								RemoveClient(i);
							}
						}
						else
						{
							buf.ReceiveBufferContent(INT_SIZE * 2, vect2);

							switch (buf.readInt32LE(INT_SIZE))
							{
							case 0:
							{
								//Join
								int packet_length = buf.readInt32LE(INT_SIZE * 0);
								int message_id = buf.readInt32LE(INT_SIZE * 1);
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());




								if (m_Rooms.find(buf.ReadString(12, buf.readInt32LE(8)).c_str()) != m_Rooms.end()) {
									m_Rooms[buf.ReadString(12, buf.readInt32LE(8)).c_str()].push_back(client);
								}
								else {
									m_Rooms[buf.ReadString(12, buf.readInt32LE(8)).c_str()] = std::vector<ClientInfo*>();
									m_Rooms[buf.ReadString(12, buf.readInt32LE(8)).c_str()].push_back(client);
								}

								printf("socket %d joined room: %s\n", (int)client->socket, buf.ReadString(12, buf.readInt32LE(8)).c_str());
								//broadcast user join message

								break;
							}
							case 1:
							{
								//Leave
								int packet_length = buf.readInt32LE(INT_SIZE * 0);
								int message_id = buf.readInt32LE(INT_SIZE * 1);
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());





								if (m_Rooms.find(buf.ReadString(12, buf.readInt32LE(8)).c_str()) != m_Rooms.end()) {
									std::string room = buf.ReadString(12, buf.readInt32LE(8)).c_str();

									for (std::vector<ClientInfo*>::iterator clientIt = m_Rooms[room].begin();
										clientIt < m_Rooms[room].end();
										clientIt++)
									{
										if (*clientIt == client)
										{
											m_Rooms[room].erase(clientIt);

											printf("socket %d left room: %s\n", (int)client->socket, room.c_str());
											//broadcast user leave message
											break;
										}
										else if (clientIt == m_Rooms[room].end() - 1)
										{
											printf("socket %d not in room: %s\n", (int)client->socket, room.c_str());
											//not in room message
										}
									}
								}
								else {
									printf("Room: %s does not exist\n", buf.ReadString(12, buf.readInt32LE(8)).c_str());
									//room desnt exist msg
								}

								break;
							}
							case 2:
							{
								//Send
								int packet_length = buf.readInt32LE(INT_SIZE * 0);
								int message_id = buf.readInt32LE(INT_SIZE * 1);
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);
								int message_length = buf.readInt32LE(INT_SIZE * 3 + room_name_length);
								std::string message = buf.ReadString(INT_SIZE * 4 + room_name_length, room_name_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());
								printf("Message Length: %i\n", message_length);
								printf("Message: %s\n", message.c_str());
								break;
							}
							case 3:
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

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Name Length: %i\n", room_name_length);
								printf("Name: %s\n", room_name.c_str());
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());
								printf("Message Length: %i\n", message_length);
								printf("Message: %s\n", message.c_str());
								break;
							}
							}

						}
					}
				}










				/*if (iRecvResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					printf("%s\n", client->dataBuf.buf);



					int iSendResult = send(client->socket, client->dataBuf.buf, client->dataBuf.len, 0);

					if (iSendResult == SOCKET_ERROR)
					{
						printf("send error %d\n", WSAGetLastError());
					}
					else if (iSendResult == 0)
					{
						printf("Send result is 0\n");
					}
					else
					{
						printf("Successfully sent %d bytes!\n", iSendResult);
					}
				}*/
			}
		}

	}



	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}