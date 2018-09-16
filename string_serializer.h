#pragma once

#include <string>
#include <vector>

class string_serializer
{
public:
	typedef std::string send_t;
	typedef send_t recv_t;

	size_t header_size()
	{
		return sizeof(uint32_t);
	}

	size_t body_size(const std::vector<char>& header)
	{
		uint32_t size = 0;

		// Extract size in big endian
		for (auto it = header.begin(); it != header.end(); ++it)
		{
			size <<= 8;
			size += *it;
		}

		return size;
	}

	bool validate_header(const std::vector<char>& header)
	{
		return header.size() == header_size();
	}

	std::vector<char> serialize(const send_t& send_msg)
	{
		// Reserve room for the header and body
		std::vector<char> buffer(header_size() + send_msg.size());

		// size_t is probably 64-bit on most systems, but 32-bit should be enough for normal uses.
		uint32_t size = send_msg.size();

		// Get the header end / body begin iterator
		auto body_begin = buffer.begin() + header_size();

		// Store size in big endian
		for (auto it = buffer.begin(); it != body_begin; ++it)
		{
			*it = (char)size;
			size >>= 8;
		}
		std::reverse(buffer.begin(), body_begin);

		// Store the string in the body portion
		std::copy(send_msg.begin(), send_msg.end(), body_begin);

		return buffer;
	}

	recv_t deserialize(const std::vector<char>& body)
	{
		return recv_t(body.begin(), body.end());
	}
};