#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <random>

#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/prepared_statement.h>

#include <cProtocol.h>
#include "AuthWebService.pb.h"

#include <openssl/sha.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "5149"

using namespace protobuf;

// Client structure
struct ClientInfo {
	SOCKET socket = NULL;
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];
std::map<std::string, std::vector<ClientInfo*>> m_Rooms;

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index]; //SHOULD ONLY EVER BE 0 so maybe do that later

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

void DisplayError(sql::SQLException exception) {
	std::cout << "# ERR: SQL Exception in " << __FILE__;
	std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	std::cout << "# ERR: " << exception.what();
	std::cout << "(MySQL error code: " << exception.getErrorCode();
	std::cout << ", SQLState: " << exception.getSQLState() << ")" << std::endl;
}


std::string hash_pass(const std::string str) {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);
	std::stringstream ss;
	for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
	}
	return ss.str();
}

void GenerateRandomNumber(size_t num, std::string &outstr) {
	std::random_device rd;
	std::uniform_int_distribution<unsigned> dist(0, 256);
	std::stringstream ss;
	for (size_t i = 0; i < num; ++i) {
		ss << (char)dist(rd);
	}
	outstr = ss.str();
}


int main(int argc, char** argv)
{
	sql::Driver* driver = nullptr;
	sql::Connection* con = nullptr;
	sql::Statement* stmt = nullptr;
	sql::PreparedStatement* prepstmt = nullptr;
	sql::ResultSet* rslt;

	std::string server = "127.0.0.1:3306";
	std::string username = "auth_serv";
	std::string password = "password";
	std::string schema = "authservdb";

	try {
		driver = get_driver_instance();
		con = driver->connect(server, username, password);
		con->setSchema(schema);
	}
	catch (sql::SQLException& exception) {
		DisplayError(exception);
		return 1;
	}

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

							switch (message_id)
							{
								case REGISTER:
								{
									int newpacket_length = buf.readInt32LE(INT_SIZE * 0);
									int newmessage_id = buf.readInt32LE(INT_SIZE * 1);
									int newmessage_length = buf.readInt32LE(INT_SIZE * 2);
									std::string newmessage = buf.ReadString(INT_SIZE * 3, newmessage_length);

									printf("Size of message: %i\n", newpacket_length);
									printf("MESSAGE ID: %i\n", newmessage_id);
									printf("MESSAGE: %s\n", newmessage.c_str());

									RegisterAccount* Registration = new RegisterAccount();

									Registration->ParseFromString(newmessage);

									std::cout << Registration->requestid() << std::endl;
									std::cout << Registration->username() << std::endl;
									std::cout << Registration->email() << std::endl;
									std::cout << Registration->password() << std::endl;

									//TODO SQL	check if exists first

									// insert new user into db
									std::stringstream ss;
									std::string usrn = Registration->username();
									std::string psw = Registration->password();
									std::string email = Registration->email();
									unsigned long requester = Registration->requestid();
									std::string salt = "";
									unsigned uid;

									try {
										// preform cleaning on the user names here.
										prepstmt = con->prepareStatement("INSERT INTO users (username, creation_date) VALUES('" + usrn + "', NOW())");
										prepstmt->executeUpdate();

										prepstmt = con->prepareStatement("SELECT ID FROM users WHERE username = '" + usrn + "'");
										rslt = prepstmt->executeQuery();

										if (rslt != 0) {
											while (rslt->next()) {
												uid = rslt->getInt(1);
												std::cout << rslt->getString(1) << std::endl;
											}
										}
										else {
											// no rows returned.

										}
									
										psw = hash_pass(psw);

										size_t RAND_SIZE = 128;
										GenerateRandomNumber(RAND_SIZE, salt);
										salt = hash_pass(salt);

										// stringstream for inserting into the web_auth table.
										ss << "INSERT INTO web_auth (User_ID, email, password, salt) VALUES (";
										ss << uid << ", '" << email << "', '" << psw << "', '" << salt << "');";

										prepstmt = con->prepareStatement(ss.str());
										prepstmt->executeUpdate();
									
										ss.str(std::string());
										std::cout << "insert operations successful" << std::endl;

										RegistrationSuccess* response = new RegistrationSuccess();
										response->set_requestid(requester);
										response->set_username(usrn);
										std::string serializedResponse = response->SerializeAsString();
										serverProto.ServerRegSuccess(serializedResponse);
										std::vector<uint8_t> vect = serverProto.GetBuffer();
										iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
										if (iResult == SOCKET_ERROR) {
											printf("send() failed with error: %d\n", WSAGetLastError());
											closesocket(client->socket);
											WSACleanup();
											return 1;
										}
									}
									catch (sql::SQLException& exception) {
										DisplayError(exception);
										//return 1;
										RequestFailure* response = new RequestFailure();
										response->set_requestid(requester);
										response->set_reason(INTERNAL_SERVER_ERROR);
										//response->set_creationdate();											
										std::string serializedResponse = response->SerializeAsString();
										serverProto.ServerRegFail(serializedResponse);
										std::vector<uint8_t> vect = serverProto.GetBuffer();
										iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
										if (iResult == SOCKET_ERROR) {
											printf("send() failed with error: %d\n", WSAGetLastError());
											closesocket(client->socket);
											WSACleanup();
											return 1;
										}
									}

									break;
								}
								case EMAILAUTH:
								{
									int newpacket_length = buf.readInt32LE(INT_SIZE * 0);
									int newmessage_id = buf.readInt32LE(INT_SIZE * 1);
									int newmessage_length = buf.readInt32LE(INT_SIZE * 2);
									std::string newmessage = buf.ReadString(INT_SIZE * 3, newmessage_length);

									printf("Size of message: %i\n", newpacket_length);
									printf("MESSAGE ID: %i\n", newmessage_id);
									printf("MESSAGE: %s\n", newmessage.c_str());

									AuthenticateAccount* AuthLogin = new AuthenticateAccount();

									AuthLogin->ParseFromString(newmessage);

									std::cout << AuthLogin->requestid() << std::endl;
									std::cout << AuthLogin->identifier() << std::endl;
									std::cout << AuthLogin->password() << std::endl;

									std::stringstream ss;
									unsigned long requester = AuthLogin->requestid();
									std::string email = AuthLogin->identifier();
									std::string psw = AuthLogin->password();
									std::string creationdate = "";
									std::string salt = "";
									std::string hpsw = "";
									std::string usrn = "";
									try {
										// preform cleaning on the user names here.
										ss << "SELECT wa.password, wa.salt, wa.username, u.creation_date FROM authservdb.web_auth wa JOIN authservdb.users u ON u.ID = wa.User_ID WHERE wa.email = '" << email << "'";
										prepstmt = con->prepareStatement(ss.str());
										rslt = prepstmt->executeQuery();

										if (rslt != 0) {
											while (rslt->next()) {
												hpsw = rslt->getString(1);
												salt = rslt->getString(2);
												usrn = rslt->getString(3);
												creationdate = rslt->getString(4);
											}
										}
										else {
											// email does not exist
											RequestFailure* response = new RequestFailure();
											response->set_requestid(requester);
											response->set_reason(INVALID_CREDENTIALS);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthFailure(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
										}

										if ((salt + hpsw) == (salt + psw)) {
											// login credentials accepted
											AuthenticationSuccess* response = new AuthenticationSuccess();
											response->set_requestid(requester);
											response->set_username(usrn);
											response->set_creationdate(creationdate);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthSuccess(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
										}
										else {
											// login credentials rejected
											RequestFailure* response = new RequestFailure();
											response->set_requestid(requester);
											response->set_reason(INVALID_PASSWORD);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthFailure(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
										}

									}
									catch (sql::SQLException& exception) {
										DisplayError(exception);
									}
									ss.str(std::string());
									break;
								}
								case USERNAMEAUTH:
								{
									int newpacket_length = buf.readInt32LE(INT_SIZE * 0);
									int newmessage_id = buf.readInt32LE(INT_SIZE * 1);
									int newmessage_length = buf.readInt32LE(INT_SIZE * 2);
									std::string newmessage = buf.ReadString(INT_SIZE * 3, newmessage_length);

									printf("Size of message: %i\n", newpacket_length);
									printf("MESSAGE ID: %i\n", newmessage_id);
									printf("MESSAGE: %s\n", newmessage.c_str());

									AuthenticateAccount* AuthLogin = new AuthenticateAccount();

									AuthLogin->ParseFromString(newmessage);

									std::cout << AuthLogin->requestid() << std::endl;
									std::cout << AuthLogin->identifier() << std::endl;
									std::cout << AuthLogin->password() << std::endl;

									std::stringstream ss;
									unsigned long requester = AuthLogin->requestid();
									std::string usrn = AuthLogin->identifier();
									std::string psw = AuthLogin->password();
									std::string creationdate = "";
									std::string salt = "";
									std::string hpsw = "";

									try {
										// preform cleaning on the user names here.
										ss << "SELECT password, salt, creation_date FROM authservdb.web_auth wa JOIN authservdb.users u ON u.ID = wa.User_ID WHERE u.username = '" << usrn << "'";
										prepstmt = con->prepareStatement(ss.str());
										rslt = prepstmt->executeQuery();

										if (rslt->next()) {
											hpsw = rslt->getString(1);
											salt = rslt->getString(2);
											creationdate = rslt->getString(3);
										}
										else {
											// user does not exist
											RequestFailure* response = new RequestFailure();
											response->set_requestid(requester);
											response->set_reason(INVALID_CREDENTIALS);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthFailure(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
											break;
										}

										psw = hash_pass(psw);


										if ((salt + hpsw) == (salt + psw)) {
											// login credentials accepted
											AuthenticationSuccess* response = new AuthenticationSuccess();
											response->set_requestid(requester);
											response->set_username(usrn);
											response->set_creationdate(creationdate);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthSuccess(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
										}
										else {
											// login credentials rejected
											RequestFailure* response = new RequestFailure();
											response->set_requestid(requester);
											response->set_reason(INVALID_PASSWORD);
											std::string serializedResponse = response->SerializeAsString();
											serverProto.ServerAuthFailure(serializedResponse);
											std::vector<uint8_t> vect = serverProto.GetBuffer();
											iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
											if (iResult == SOCKET_ERROR) {
												printf("send() failed with error: %d\n", WSAGetLastError());
												closesocket(client->socket);
												WSACleanup();
												return 1;
											}
										}

									}
									catch (sql::SQLException& exception) {
										DisplayError(exception);
									}


									break;
								}
								case DISCONNECT:
								{
									int newpacket_length = buf.readInt32LE(INT_SIZE * 0);
									int newmessage_id = buf.readInt32LE(INT_SIZE * 1);
									int newmessage_length = buf.readInt32LE(INT_SIZE * 2);
									std::string newmessage = buf.ReadString(INT_SIZE * 3, newmessage_length);

									printf("Size of message: %i\n", newpacket_length);
									printf("MESSAGE ID: %i\n", newmessage_id);
									printf("MESSAGE: %s\n", newmessage.c_str());

									Disconnect* disconnect = new Disconnect();

									disconnect->ParseFromString(newmessage);

									std::cout << disconnect->requestid() << std::endl;
									std::cout << disconnect->username() << std::endl;

									unsigned long requester = disconnect->requestid();
									std::string usrn = disconnect->username();

									try {
										std::stringstream ss;

										// preform cleaning on the user names here.
										ss << "UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE username = '" << usrn << "'";
										prepstmt = con->prepareStatement(ss.str());
										rslt = prepstmt->executeQuery();

										// login credentials accepted
										DisconnectSuccess* response = new DisconnectSuccess();
										response->set_requestid(requester);
										response->set_username(usrn);
										//response->set_creationdate();										
										std::string serializedResponse = response->SerializeAsString();
										serverProto.ServerDiscSuccess(serializedResponse);
										std::vector<uint8_t> vect = serverProto.GetBuffer();
										iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
										if (iResult == SOCKET_ERROR) {
											printf("send() failed with error: %d\n", WSAGetLastError());
											closesocket(client->socket);
											WSACleanup();
											return 1;
										}
									}
									catch (sql::SQLException& exception) {
										DisplayError(exception);
										//return 1;
										RequestFailure* response = new RequestFailure();
										response->set_requestid(requester);
										response->set_reason(INTERNAL_SERVER_ERROR);
										//response->set_creationdate();											
										std::string serializedResponse = response->SerializeAsString();
										serverProto.ServerDiscFailure(serializedResponse);
										std::vector<uint8_t> vect = serverProto.GetBuffer();
										iResult = send(client->socket, (char*)vect.data(), (int)vect.size(), 0);
										if (iResult == SOCKET_ERROR) {
											printf("send() failed with error: %d\n", WSAGetLastError());
											closesocket(client->socket);
											WSACleanup();
											return 1;
										}
									}


									break;
								}
								default:
									printf("You what mate?\n");
									break;
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