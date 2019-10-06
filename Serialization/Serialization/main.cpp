#include <iostream>
#include <bitset>
#include <vector>

//We send data over the network as whole BYTES

//char a;				1 byte,		8 bits
//short b;				2 bytes,	16 bits
//int c;				4 bytes,	32 bits
//unsigned char d;		1 byte,		8 bits
//unsigned short e;		2 bytes,	16 bits
//unsigned int f;		4 bytes,	32 bits

class Buffer
{
private:
	std::vector<uint8_t> _buffer;

public:
	Buffer(size_t size)
	{
		for (int i = 0; i < size; i++)
		{
			_buffer.push_back(0);
		}
	}

	void writeInt32LE(size_t index, int32_t value)
	{
		if (index + sizeof(int32_t) > _buffer.size())
		{
			for (size_t i = 0; i < _buffer.size() - index - sizeof(int32_t); i++)
			{
				_buffer.push_back(0);
			}
		}
		_buffer[index] = value;
		_buffer[index + 1] = value >> 8;
		_buffer[index + 2] = value >> 16;
		_buffer[index + 3] = value >> 24;
	}

	void writeInt32LE(int32_t value)
	{
		if (sizeof(int32_t) > _buffer.size())
		{
			for (size_t i = 0; i < _buffer.size() - sizeof(int32_t); i++)
			{
				_buffer.push_back(0);
			}
		}
		_buffer[0] = value;
		_buffer[1] = value >> 8;
		_buffer[2] = value >> 16;
		_buffer[3] = value >> 24;
	}

	void writeShortLE(size_t index, int16_t value)
	{
		if (index + sizeof(int16_t) > _buffer.size())
		{
			for (size_t i = 0; i < _buffer.size() - index - sizeof(int16_t); i++)
			{
				_buffer.push_back(0);
			}
		}
		_buffer[index] = value;
		_buffer[index + 1] = value >> 8;
	}

	void writeShortLE(int16_t value)
	{
		if (sizeof(int16_t) > _buffer.size())
		{
			for (size_t i = 0; i < _buffer.size() - sizeof(int16_t); i++)
			{
				_buffer.push_back(0);
			}
		}
		_buffer[0] = value;
		_buffer[1] = value >> 8;
	}

	void WriteString(size_t index, std::string value)
	{
		for (size_t i = 0; i < value.length(); i++)
		{
			_buffer[index + i] = value[i];
		}
	}

	void WriteString(std::string value)
	{
		for (size_t i = 0; i < value.length(); i++)
		{
			_buffer[i] = value[i];
		}
	}

	uint32_t readInt32LE(size_t index)
	{
		uint32_t swapped = 0;

		swapped |= _buffer[index + 3] << 24;
		swapped |= _buffer[index + 2] << 16;
		swapped |= _buffer[index + 1] << 8;
		swapped |= _buffer[index] << 0;

		return swapped;
	}

	uint32_t readInt32LE()
	{
		uint32_t swapped = 0;

		swapped |= _buffer[3] << 24;
		swapped |= _buffer[2] << 16;
		swapped |= _buffer[1] << 8;
		swapped |= _buffer[0] << 0;

		return swapped;
	}

	uint16_t readShortLE(size_t index)
	{
		uint16_t swapped = 0;

		swapped |= _buffer[index + 1] << 8;
		swapped |= _buffer[index] << 0;

		return swapped;
	}

	uint16_t readShortLE()
	{
		uint16_t swapped = 0;

		swapped |= _buffer[1] << 8;
		swapped |= _buffer[0] << 0;

		return swapped;
	}

	std::string ReadString(size_t index, uint8_t length)
	{
		std::string swapped;

		for (size_t i = 0; i < length; i++)
		{
			swapped.push_back(_buffer[index + i]);
		}

		return swapped;
	}

	std::string ReadString(uint8_t length)
	{
		std::string swapped;

		for (size_t i = 0; i < length; i++)
		{
			swapped.push_back(_buffer[i]);
		}

		return swapped;
	}
};

int main(int argc, char** argv)
{
	Buffer buf(0);

	buf.writeShortLE(50);
	std::cout << buf.readShortLE() << std::endl;

	buf.writeShortLE(4, 500);
	std::cout << buf.readShortLE(4) << std::endl;

	buf.writeInt32LE(50);
	std::cout << buf.readInt32LE() << std::endl;

	buf.writeInt32LE(4, 500);
	std::cout << buf.readInt32LE(4) << std::endl;

	buf.WriteString(0, "Hello");
	std::cout << buf.ReadString(0, 4) << std::endl;

	//Wait for user input
	system("Pause");
	return 0;
}