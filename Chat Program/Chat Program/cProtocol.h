#include "cBuffer.h"

class Protocol
{
private:
	Buffer buffer = Buffer(32);

	unsigned int message_id = 0;

public:
	Protocol()
	{
		//Don't think we need to do anything here?
	}

	void SendMessage(std::string room, std::string message)
	{
		// [Header] [length] [room_name] [length] [message]

	}

	void RecieveMessage(std::string name, std::string room, std::string message)
	{
		// [Header] [length] [name] [length] [room_name] [length] [message]

	}

	void JoinRoom(std::string room)
	{
		// [Header] [length] [room_name]
		buffer.readInt32LE(room.length());
		buffer.ReadString(room);
	}

	void LeaveRoom(std::string room)
	{
		// [Header] [length] [room_name]

	}

	void Header()
	{
		message_id++;

		//add up everything else for Package length
	}


};