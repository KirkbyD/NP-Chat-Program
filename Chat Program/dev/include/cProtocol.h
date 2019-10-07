#include "cBuffer.h"

#define INT_SIZE sizeof(int32_t)/sizeof(char)

class Protocol
{
private:
<<<<<<< HEAD
	Buffer buffer;
	unsigned int message_id;
=======
	Buffer buffer = Buffer(0);
	unsigned int message_id = 0;
>>>>>>> bb40fb4bae4a42550fd5306ae094cf2a054cb03e

public:
	Protocol()
	{
		buffer = Buffer(0);
		message_id = 0;
	}

	Protocol(unsigned int m_id, unsigned int buffer_size)
	{
		buffer = Buffer(buffer_size);
		message_id = m_id;
	}

<<<<<<< HEAD
	std::vector<uint8_t> GetBuffer()
	{
		return buffer.GetBufferContent();
	}

	std::vector<uint8_t> UserSendMessage(std::string room, std::string message)
=======
	Buffer SendMessage(std::string room, std::string message)
>>>>>>> bb40fb4bae4a42550fd5306ae094cf2a054cb03e
	{
		buffer.Clear();

		// [Header] [length] [room_name] [length] [message]
		buffer.writeInt32LE(INT_SIZE * 4 + room.length() + message.length());
		buffer.writeInt32LE(INT_SIZE, message_id++);

		//room
		buffer.writeInt32LE(INT_SIZE * 2, room.length());
		buffer.WriteString(INT_SIZE * 3, room);
		
		//message
		buffer.writeInt32LE(INT_SIZE * 3 + room.length(), message.length());
		buffer.WriteString(INT_SIZE * 4 + room.length(), message);

<<<<<<< HEAD
		return GetBuffer();
=======
		return buffer;
>>>>>>> bb40fb4bae4a42550fd5306ae094cf2a054cb03e
	}

	std::vector<uint8_t> UserRecieveMessage(std::string name, std::string room, std::string message)
	{
		buffer.Clear();

		// [Header] [length] [name] [length] [room_name] [length] [message]
		buffer.writeInt32LE(INT_SIZE * 5 + name.length() + room.length() + message.length());
		buffer.writeInt32LE(INT_SIZE, message_id++);

		//name
		buffer.writeInt32LE(INT_SIZE * 2, name.length());
		buffer.WriteString(INT_SIZE * 3, name);

		//room
		buffer.writeInt32LE(INT_SIZE * 3 + name.length(), room.length());
		buffer.WriteString(INT_SIZE * 4 + name.length(), room);

		//message
		buffer.writeInt32LE(INT_SIZE * 4 + room.length() + name.length(), message.length());
		buffer.WriteString(INT_SIZE * 5 + room.length() + name.length(), message);

		return GetBuffer();
	}

<<<<<<< HEAD
	std::vector<uint8_t> UserJoinRoom(std::string room)
=======
	Buffer JoinRoom(std::string name, std::string room)
	{
		buffer.Clear();

		// [Header] [length] [user_name] [length] [room_name]
		buffer.writeInt32LE(INT_SIZE * 3 + room.length());
		buffer.writeInt32LE(INT_SIZE, message_id++);

		//name
		buffer.writeInt32LE(INT_SIZE * 2, name.length());
		buffer.WriteString(INT_SIZE * 3, name);

		//room
		buffer.writeInt32LE(INT_SIZE * 3 + name.length(), room.length());
		buffer.WriteString(INT_SIZE * 4 + name.length(), room);

		return buffer;
	}

	Buffer LeaveRoom(std::string room)
>>>>>>> bb40fb4bae4a42550fd5306ae094cf2a054cb03e
	{
		buffer.Clear();

		// [Header] [length] [room_name]
		buffer.writeInt32LE(INT_SIZE * 3 + room.length());
		buffer.writeInt32LE(INT_SIZE, message_id++);

		buffer.writeInt32LE(INT_SIZE * 2, room.length());
		buffer.WriteString(INT_SIZE * 3, room);

<<<<<<< HEAD
		return GetBuffer();
=======
		return buffer;
>>>>>>> bb40fb4bae4a42550fd5306ae094cf2a054cb03e
	}

	std::vector<uint8_t> UserLeaveRoom(std::string room)
	{
		buffer.Clear();

		// [Header] [length] [room_name]
		//packet length
		buffer.writeInt32LE(INT_SIZE * 3 + room.length());
		//message_id
		buffer.writeInt32LE(INT_SIZE, message_id++);

		buffer.writeInt32LE(INT_SIZE * 2, room.length());
		buffer.WriteString(INT_SIZE * 3, room);

		return GetBuffer();
	}
};