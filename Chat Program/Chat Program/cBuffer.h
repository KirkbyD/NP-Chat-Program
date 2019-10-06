#include <bitset>
#include <vector>

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
		_buffer[index] = value;
		_buffer[index + 1] = value >> 8;
		_buffer[index + 2] = value >> 16;
		_buffer[index + 3] = value >> 24;
	}

	void writeInt32LE(int32_t value)
	{
		_buffer[0] = value;
		_buffer[1] = value >> 8;
		_buffer[2] = value >> 16;
		_buffer[3] = value >> 24;
	}

	void writeShortLE(size_t index, int16_t value)
	{
		_buffer[index] = value;
		_buffer[index + 1] = value >> 8;
	}

	void writeShortLE(int16_t value)
	{
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