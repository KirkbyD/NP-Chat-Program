#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <map>

#include <cProtocol.h>
#include "AuthWebService.pb.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "5150"
#define AUTH_PORT "5149"


#define UINT32_SIZE sizeof(uint32_t)/sizeof(char)

using namespace protobuf;

// Client structure
struct ClientInfo {
	SOCKET socket;
	std::string username;
	bool loggedIn;
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];
std::map<std::string, std::vector<ClientInfo*>> m_Rooms;

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	
	//remove client from all vectors in room map
	for (std::map<std::string, std::vector<ClientInfo*>>::iterator mapIt = m_Rooms.begin();
		mapIt != m_Rooms.end();
		mapIt++)
	{
		for (std::vector<ClientInfo*>::iterator clientIt = mapIt->second.begin();
			clientIt < mapIt->second.end();
			clientIt++)
		{
			if (*clientIt == client)
			{
				mapIt->second.erase(clientIt);
				break;
			}
		}
	}

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
	/***************************************Example*****************************************/
	/***************************************************************************************/
	//AddressBook addressBook;

	//Person* newPerson1 = addressBook.add_people();
	//CreatePerson(
	//	newPerson1,
	//	1,
	//	"l_gustafson@fanshaweonline.ca",
	//	"password");

	//// Serialize the data into a string
	//std::string serializedAddressBook = addressBook.SerializeAsString();
	//std::cout << serializedAddressBook << std::endl;
	//// This is the info you would send over the network

	//// Receive the string/data, and deserialize
	//AddressBook received;
	//received.ParseFromString(serializedAddressBook);

	//// Check data
	//int numPeople = received.people_size();
	//printf("Received %d people in our address book!\n", numPeople);

	//// pause
	//system("Pause");
	/***************************************************************************************/

	WSADATA wsaData;
	int iResult;
	Protocol serverProto = Protocol();

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


	//CLIENT CONNECTION TO AUTH SERVER
	SOCKET connectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	{
		// resolve the server address and port
		iResult = getaddrinfo("127.0.0.1", AUTH_PORT, &hints, &result);
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
	}

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
					info->loggedIn = false;
					info->username = "Anon";
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
				for (int i = 0; i < INT_SIZE * 2; i++)
				{
					vect.push_back('0');
				}
				iResult = recv(client->socket, (char*)vect.data(), INT_SIZE * 2, 0);

				if (iResult == SOCKET_ERROR)
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
					if (iResult == 0)
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

						iResult = recv(client->socket, (char*)vect2.data(), (int)vect2.size(), 0);

						if (iResult == SOCKET_ERROR)
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

							int packet_length = buf.readInt32LE(INT_SIZE * 0);
							int message_id = buf.readInt32LE(INT_SIZE * 1);

							if (!client->loggedIn
								&& (message_id != REGISTER
									&& message_id != EMAILAUTH
									&& message_id != USERNAMEAUTH))
								break;

							switch (message_id)
							{
							case JOIN:
							{
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());

								if (m_Rooms.find(room_name.c_str()) != m_Rooms.end()) {
									m_Rooms[room_name.c_str()].push_back(client);
								}
								else {
									m_Rooms[room_name.c_str()] = std::vector<ClientInfo*>();
									m_Rooms[room_name.c_str()].push_back(client);
								}

								printf("socket %d joined room: %s\n", (int)client->socket, room_name.c_str());
								
								std::string msg = "User '" + std::to_string(client->socket) + "' has joined " + room_name;
								serverProto.UserRecieveMessage("Server", room_name.c_str(), msg);

								std::vector<uint8_t> vect = serverProto.GetBuffer();

								for (ClientInfo* tmpClient : m_Rooms[room_name.c_str()]) {
									iResult = send(tmpClient->socket, (char*)vect.data(), (int)vect.size(), 0);

									if (iResult == SOCKET_ERROR)
									{
										printf("send error %d\n", WSAGetLastError());
									}
									else
									{
										printf("Bytes sent: %d\n", iResult);
									}
								}

								break;
							}
							case LEAVE:
							{
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());

								if (m_Rooms.find(room_name.c_str()) != m_Rooms.end()) {
									for (std::vector<ClientInfo*>::iterator clientIt = m_Rooms[room_name].begin();
										clientIt < m_Rooms[room_name].end();
										clientIt++)
									{
										if (*clientIt == client)
										{
											printf("socket %d left room: %s\n", (int)client->socket, room_name.c_str());
											
											std::string msg = "User '" + std::to_string(client->socket) + "' has left " + room_name;
											serverProto.UserRecieveMessage("Server", room_name.c_str(), msg);

											std::vector<uint8_t> vect = serverProto.GetBuffer();

											for (ClientInfo* tmpClient : m_Rooms[room_name.c_str()]) {
												iResult = send(tmpClient->socket, (char*)vect.data(), (int)vect.size(), 0);

												if (iResult == SOCKET_ERROR)
												{
													printf("send error %d\n", WSAGetLastError());
												}
												else
												{
													printf("Bytes sent: %d\n", iResult);
												}
											}
											m_Rooms[room_name].erase(clientIt);

											break;
										}
										else if (clientIt == m_Rooms[room_name].end() - 1)
										{
											printf("socket %d not in room: %s\n", (int)client->socket, room_name.c_str());
										}
									}
								}
								else {
									printf("Room: %s does not exist\n", room_name.c_str());
								}

								break;
							}
							case SEND:
							{
								int room_name_length = buf.readInt32LE(INT_SIZE * 2);
								std::string room_name = buf.ReadString(INT_SIZE * 3, room_name_length);
								int message_length = buf.readInt32LE(INT_SIZE * 3 + room_name_length);
								std::string message = buf.ReadString(INT_SIZE * 4 + room_name_length, message_length);

								printf("Packet Length: %i\n", packet_length);
								printf("Message ID: %i\n", message_id);
								printf("Room Name Length: %i\n", room_name_length);
								printf("Room Name: %s\n", room_name.c_str());
								printf("Message Length: %i\n", message_length);
								printf("Message: %s\n", message.c_str());

								std::string sender = std::to_string(client->socket);

								serverProto.UserRecieveMessage(sender, room_name, message);

								std::vector<uint8_t> vect = serverProto.GetBuffer();

								if (m_Rooms.find(room_name.c_str()) != m_Rooms.end()) {
									for (std::vector<ClientInfo*>::iterator clientIt = m_Rooms[room_name].begin();
										clientIt < m_Rooms[room_name].end();
										clientIt++)
									{
										if (*clientIt == client)
										{
											for (ClientInfo* tmpClient : m_Rooms[room_name]) {
												iResult = send(tmpClient->socket, (char*)vect.data(), (int)vect.size(), 0);

												if (iResult == SOCKET_ERROR)
												{
													printf("send error %d\n", WSAGetLastError());
												}
												else
												{
													printf("Bytes sent: %d\n", iResult);
												}
											}
										}
									}
								}

								break;
							}
							case REGISTER:
							{
								int user_length = buf.readInt32LE(INT_SIZE * 2);
								std::string username = buf.ReadString(INT_SIZE * 3, user_length);

								int email_length = buf.readInt32LE(INT_SIZE * 3 + user_length);
								std::string email = buf.ReadString(INT_SIZE * 4 + user_length, email_length);

								int pass_length = buf.readInt32LE(INT_SIZE * 4 + user_length + email_length);
								std::string password = buf.ReadString(INT_SIZE * 5 + user_length + email_length, pass_length);

								printf("REGISTER command recieved: user:%s, email:%s, pass:%s\n", username.c_str(), email.c_str(), password.c_str());

								//Check if registration was successful or it failed
								/*****************/								
								RegisterAccount* Registration = new RegisterAccount();

								Registration->set_requestid(client->socket);
								Registration->set_username(username);
								Registration->set_email(email);
								Registration->set_password(password);

								int RegLength = Registration->ByteSizeLong();
								std::cout << "Size is: " << RegLength << std::endl;
								std::string serializedReg = Registration->SerializeAsString();
								std::cout << serializedReg << std::endl;

								/*Create header the same way we used to do that*/
								/*We can get size of message from CAWlength + INT_SIZE * 2*/
								/*To use id of message we have to add more enums in cProtocol.h*/
								serverProto.ServerRegister(serializedReg);
								std::vector<uint8_t> vect = serverProto.GetBuffer();

								/*Send Header + serializedCAW to auth_server*/
								iResult = send(connectSocket, (char*)vect.data(), (int)vect.size(), 0);
								if (iResult == SOCKET_ERROR)
								{
									printf("send() failed with error: %d\n", WSAGetLastError());
									closesocket(connectSocket);
									WSACleanup();
									return 1;
								}
								break;
							}
							case EMAILAUTH:
							{
								//TODO
								int email_length = buf.readInt32LE(INT_SIZE * 2);
								std::string email = buf.ReadString(INT_SIZE * 3, email_length);
								int pass_length = buf.readInt32LE(INT_SIZE * 3 + email_length);
								std::string password = buf.ReadString(INT_SIZE * 4 + email_length, pass_length);

								printf("EMAILAUTH command recieved: email:%s, pass:%s", email.c_str(), password.c_str());
								break;
							}
							case USERNAMEAUTH:
							{
								//TODO
								int user_length = buf.readInt32LE(INT_SIZE * 2);
								std::string user = buf.ReadString(INT_SIZE * 3, user_length);
								int pass_length = buf.readInt32LE(INT_SIZE * 3 + user_length);
								std::string password = buf.ReadString(INT_SIZE * 4 + user_length, pass_length);

								printf("USERNAMEAUTH command recieved: user:%s, pass:%s", user.c_str(), password.c_str());
								break;
							}
							case REGISTERSUCCESS:
							{
								/*Receive data and deserialize into result and reason*/
								bool result = false;
								ReasonError reason = INTERNAL_SERVER_ERROR;
								std::vector<uint8_t> vect1;

								/*Success*/
								Buffer anotherbuf;
								anotherbuf.ReceiveBufferContent(vect1);

								int anotherpacket_length = anotherbuf.readInt32LE(INT_SIZE * 0);
								int anothermessage_id = anotherbuf.readInt32LE(INT_SIZE * 1);
								int anothermessage_length = anotherbuf.readInt32LE(INT_SIZE * 2);
								std::string anothermessage = anotherbuf.ReadString(INT_SIZE * 3, anothermessage_length);

								//CreateAccountWebSuccess* receivedexample1 = new CreateAccountWebSuccess();
								//receivedexample1->ParseFromString(anothermessage);

								//std::cout << receivedexample1->requestid() << std::endl;
								//std::cout << receivedexample1->userid() << std::endl;

								result = true;
								/*Success*/
								break;
							}
							case REGISTERFAILURE:
							{
								/*Receive data and deserialize into result and reason*/
								bool result = false;
								ReasonError reason = INTERNAL_SERVER_ERROR;
								std::vector<uint8_t> vect1;

								/*Failure*/
								Buffer anotherbuf;
								anotherbuf.ReceiveBufferContent(vect2);

								int anotherpacket_length = anotherbuf.readInt32LE(INT_SIZE * 0);
								int anothermessage_id = anotherbuf.readInt32LE(INT_SIZE * 1);
								int anothermessage_length = anotherbuf.readInt32LE(INT_SIZE * 2);
								std::string anothermessage = anotherbuf.ReadString(INT_SIZE * 3, anothermessage_length);

								//CreateAccountWebFailure* receivedexample2 = new CreateAccountWebFailure();
								//receivedexample2->ParseFromString(anothermessage);

								//std::cout << receivedexample2->requestid() << std::endl;
								//std::cout << receivedexample2->reason() << std::endl;

								result = false;
								//reason = receivedexample2->reason();
								/*Failure*/
								break;
							}
							case AUTHSUCCESS:
								break;
							case AUTHFAILURE:
								break;	
							default:
								printf("You what mate?\n");
							}
						}
					}
				}
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