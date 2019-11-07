#include "cBuffer.h"

#define INT_SIZE sizeof(int32_t)/sizeof(char)

enum MESSAGE_ID { JOIN, LEAVE, SEND, RECIEVE, REGISTER, AUTHENTICATE };

class Protocol
{
private:
	Buffer buffer;

public:
	Protocol()
	{
		buffer = Buffer(0);
	}

	Protocol(unsigned int buffer_size)
	{
		buffer = Buffer(buffer_size);
	}

	//change to using bitsets. Called in client based on recieved enum
	std::vector<uint8_t> GetBuffer()
	{
		return buffer.GetBufferContent();
	}

	void UserSendMessage(std::string room, std::string message)
	{
		buffer.Clear();

		// [Header] [length] [room_name] [length] [message]

		//packet length
		buffer.writeInt32LE(0, SwapIntEndian(INT_SIZE * 4 + room.length() + message.length()));
		//message_id
		buffer.writeInt32LE(INT_SIZE, SwapIntEndian(SEND));

		//room
		buffer.writeInt32LE(INT_SIZE * 2, SwapIntEndian(room.length()));
		buffer.WriteString(INT_SIZE * 3, room);
		
		//message
		buffer.writeInt32LE(INT_SIZE * 3 + room.length(), SwapIntEndian(message.length()));
		buffer.WriteString(INT_SIZE * 4 + room.length(), message);

		return;
	}

	void UserRecieveMessage(std::string name, std::string room, std::string message)
	{
		buffer.Clear();

		// [Header] [length] [name] [length] [room_name] [length] [message]

		//packet length
		buffer.writeInt32LE(0, SwapIntEndian(INT_SIZE * 5 + name.length() + room.length() + message.length()));
		//message_id
		buffer.writeInt32LE(INT_SIZE, SwapIntEndian(RECIEVE));

		//name
		buffer.writeInt32LE(INT_SIZE * 2, SwapIntEndian(name.length()));
		buffer.WriteString(INT_SIZE * 3, name);

		//room
		buffer.writeInt32LE(INT_SIZE * 3 + name.length(), SwapIntEndian(room.length()));
		buffer.WriteString(INT_SIZE * 4 + name.length(), room);

		//message
		buffer.writeInt32LE(INT_SIZE * 4 + room.length() + name.length(), SwapIntEndian(message.length()));
		buffer.WriteString(INT_SIZE * 5 + room.length() + name.length(), message);

		return;
	}

	void UserJoinRoom(std::string room)
	{
		buffer.Clear();

		// [Header] [length] [room_name]

		//packet length
		buffer.writeInt32LE(0, SwapIntEndian(INT_SIZE * 3 + room.length()));
		//message_id
		buffer.writeInt32LE(INT_SIZE, SwapIntEndian(JOIN));

		//room
		buffer.writeInt32LE(INT_SIZE * 2, SwapIntEndian(room.length()));
		buffer.WriteString(INT_SIZE * 3, room);

		return;
	}

	void UserLeaveRoom(std::string room)
	{
		buffer.Clear();

		// [Header] [length] [room_name]

		//packet length
		buffer.writeInt32LE(0, INT_SIZE * SwapIntEndian(3 + room.length()));
		//message_id
		buffer.writeInt32LE(INT_SIZE, SwapIntEndian(LEAVE));

		//room
		buffer.writeInt32LE(INT_SIZE * 2, SwapIntEndian(room.length()));
		buffer.WriteString(INT_SIZE * 3, room);

		return;
	}

	int SwapIntEndian(int value)
	{
		// We need to grab the first byte an move it to the last
		// Bytes in order: A B C D
		char A = value >> 24;
		char B = value >> 16;
		char C = value >> 8;
		char D = value;

		// OR the data into our swapped variable
		int swapped = 0;
		swapped |= D << 24;
		swapped |= C << 16;
		swapped |= B << 8;
		swapped |= A;

		return swapped;
	}

	short SwapShortEndian(int value)
	{
		// We need to grab the first byte an move it to the last
		// Bytes in order: A B
		char A = value >> 8;
		char B = value;

		// OR the data into our swapped variable
		int swapped = 0;
		swapped |= B << 8;
		swapped |= A;

		return swapped;
	}

	void ClearBuffer() 
	{
		buffer.Clear();
	}
};