#include <bitset>
#include <vector>

class Buffer
{
private:
	std::vector<uint8_t> _buffer;
	int _readIndex;
	int _writeIndex;

public:
	Buffer()
	{
		this->_buffer = std::vector<uint8_t>(0);
		this->_readIndex = 0;
		this->_writeIndex = 0;
	}

	Buffer(size_t size)
	{
		this->_buffer = std::vector<uint8_t>(0);
		for (unsigned int i = 0; i < size; i++)
		{
			this->_buffer.push_back(0);
		}
		this->_readIndex = 0;
		this->_writeIndex = 0;
	}

	void Clear()
	{
		this->_buffer.clear();
	}

	void writeInt32LE(size_t index, int32_t value)
	{
		if (index + sizeof(int32_t) > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - index - sizeof(int32_t); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		this->_buffer[index] = value;
		this->_buffer[index + 1] = value >> 8;
		this->_buffer[index + 2] = value >> 16;
		this->_buffer[index + 3] = value >> 24;
		this->_writeIndex = index + 4;
	}

	void writeInt32LE(int32_t value)
	{
		if (this->_writeIndex + sizeof(int32_t) > this->_buffer.size())
		{
			for (size_t i = this->_writeIndex; i < this->_buffer.size() - sizeof(int32_t); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		
		this->_buffer[_writeIndex] = value;
		this->_buffer[_writeIndex + 1] = value >> 8;
		this->_buffer[_writeIndex + 2] = value >> 16;
		this->_buffer[_writeIndex + 3] = value >> 24;
		this->_writeIndex += 4;
	}

	void writeShortLE(size_t index, int16_t value)
	{
		if (index + sizeof(int16_t) > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - index - sizeof(int16_t); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		this->_buffer[index] = value;
		this->_buffer[index + 1] = value >> 8;
		this->_writeIndex = index + 2;
	}

	void writeShortLE(int16_t value)
	{
		if (this->_writeIndex + sizeof(int16_t) > this->_buffer.size())
		{
			for (size_t i = this->_writeIndex; i < this->_buffer.size() - sizeof(int16_t); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		this->_buffer[0] = value;
		this->_buffer[1] = value >> 8;
		this->_writeIndex += 2;
	}

	void WriteString(size_t index, std::string value)
	{
		if (index + value.length() > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - index - value.length(); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		this->_writeIndex = index;
		for (size_t i = 0; i < value.length(); i++)
		{
			this->_buffer[this->_writeIndex++] = value[i];
		}
	}

	void WriteString(std::string value)
	{
		if (this->_writeIndex + value.length() > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - value.length(); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		for (size_t i = 0; i < value.length(); i++)
		{
			this->_buffer[this->_writeIndex++] = value[i];
		}
	}

	uint32_t readInt32LE(size_t index)
	{
		uint32_t swapped = 0;

		swapped |= this->_buffer[index] << 24;
		swapped |= this->_buffer[index + 1] << 16;
		swapped |= this->_buffer[index + 2] << 8;
		swapped |= this->_buffer[index + 3] << 0;

		/*swapped |= this->_buffer[index + 3] << 24;
		swapped |= this->_buffer[index + 2] << 16;
		swapped |= this->_buffer[index + 1] << 8;
		swapped |= this->_buffer[index] << 0;*/

		this->_readIndex = index + 4;

		return swapped;
	}

	uint32_t readInt32LE()
	{
		uint32_t swapped = 0;

		swapped |= _buffer[this->_readIndex + 3] << 24;
		swapped |= _buffer[this->_readIndex + 2] << 16;
		swapped |= _buffer[this->_readIndex + 1] << 8;
		swapped |= _buffer[this->_readIndex + 0] << 0;

		this->_readIndex += 4;

		return swapped;
	}

	uint16_t readShortLE(size_t index)
	{
		uint16_t swapped = 0;

		swapped |= this->_buffer[index + 1] << 8;
		swapped |= this->_buffer[index] << 0;

		this->_readIndex += 2;

		return swapped;
	}

	uint16_t readShortLE()
	{
		uint16_t swapped = 0;

		swapped |= _buffer[this->_readIndex + 1] << 8;
		swapped |= _buffer[this->_readIndex + 0] << 0;

		this->_readIndex += 2;

		return swapped;
	}

	std::string ReadString(size_t index, uint8_t length)
	{
		std::string swapped = "";
		
		this->_readIndex = index;

		for (size_t i = 0; i < length; i++)
		{
			swapped += this->_buffer[this->_readIndex++];
		}

		return swapped;
	}

	std::string ReadString(uint8_t length)
	{
		std::string swapped = "";

		for (size_t i = 0; i < length; i++)
		{
			swapped += this->_buffer[this->_readIndex++];
		}

		return swapped;
	}

	std::vector<uint8_t> GetBufferContent()
	{
		std::vector<uint8_t> content;

		for (size_t i = 0; i < this->_buffer.size(); i++)
		{
			content.push_back(this->_buffer[i]);
		}

		return content;
	}

	void ReceiveBufferContent(std::vector<uint8_t> content)
	{
		if (this->_writeIndex + content.size() > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - content.size(); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		for (size_t i = 0; i < content.size(); i++)
		{
			this->_buffer[this->_writeIndex++] = content[i];
		}
	}

	void ReceiveBufferContent(size_t index, std::vector<uint8_t> content)
	{
		/*if (index + content.size() > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - index - content.size(); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		for (size_t i = 0; i < content.size(); i++)
		{
			this->_buffer[i + index] = content[i + index];
		}*/

		this->_writeIndex = index;
		if (this->_writeIndex + content.size() > this->_buffer.size())
		{
			for (size_t i = 0; i < this->_buffer.size() - index - content.size(); i++)
			{
				this->_buffer.push_back(0);
			}
		}
		for (size_t i = 0; i < content.size(); i++)
		{
			this->_buffer[this->_writeIndex++] = content[i];
		}
	}
};
