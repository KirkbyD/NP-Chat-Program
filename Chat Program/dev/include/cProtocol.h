#include "cBuffer.h"

#define INT_SIZE sizeof(int32_t)/sizeof(char)

class Protocol
{
private:
	Buffer buffer = Buffer(0);
	unsigned int message_id;

public:
	Protocol() { }

	Protocol(unsigned int m_id, unsigned int buffer_size)
	{
		buffer = Buffer(buffer_size);
		message_id = m_id;
	}

	void SendMessage(std::string room, std::string message)
	{
		buffer.Clear();

		// [Header] [length] [room_name] [length] [message]
		buffer.writeInt32LE(INT_SIZE * 4 + room.length() + message.length());
		//buffer.
		buffer.writeInt32LE(INT_SIZE, message_id++);

		//room
		buffer.writeInt32LE(INT_SIZE * 2, room.length());
		buffer.WriteString(INT_SIZE * 3, room);
		
		//message
		buffer.writeInt32LE(INT_SIZE * 3 + room.length(), message.length());
		buffer.WriteString(INT_SIZE * 4 + room.length(), message);
	}

	Buffer RecieveMessage(std::string name, std::string room, std::string message)
	{
		buffer.Clear();

		// [Header] [length] [name] [length] [room_name] [length] [message]
		buffer.writeInt32LE(INT_SIZE * 5 + name.length() + room.length() + message.length());
		//buffer.
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

		return buffer;
	}

	void ChangeRoom(std::string room)
	{
		buffer.Clear();

		// [Header] [length] [room_name]
		//packet length
		buffer.writeInt32LE(INT_SIZE * 3 + room.length());
		//message_id
		buffer.writeInt32LE(INT_SIZE, message_id++);

		buffer.writeInt32LE(INT_SIZE * 2, room.length());
		buffer.WriteString(INT_SIZE * 3, room);
	}

	uint8_t* GetBuffer()
	{
		return buffer.GetBufferContent();
	}
};