/**
@author Gary Heckman
@file string_serializer.h
@brief Simple string serializer example
@detail
	Example serializer for use with the boost messaging library. It fulfils the
	Serializer concept. See README for more information.
*/

#pragma once

#include <string>
#include <vector>

namespace boost_messaging
{
	/**
	Simple string serializer example.
	Serializes and deserializes strings.
	Prepends a 4-byte header with the size of the string in bytes.
	*/
	class string_serializer
	{
	public:
		typedef std::string send_t;
		typedef send_t recv_t;

		/**
		Gets the header size.
		This is the size of the first part of the message for tcp,
		or the offset into the body for udp.
		@return The header size.
		*/
		constexpr size_t header_size() const
		{
			return sizeof(uint32_t);
		}

		/**
		Gets the body size.
		This is the size of the second message read in for tcp,
		or the remaining message size for udp.
		@tparam FwdIter Forward iterator
		@param [in] first Iterator to the first character in the header
		@param [in] last  Iterator past the last character in the header
		*/
		template <typename FwdIter>
		size_t body_size(FwdIter first, FwdIter last) const
		{
			uint32_t size = 0;

			// Extract size in big endian
			for (; first != last; ++first)
			{
				size <<= 8;
				size += *first;
			}

			return size;
		}

		/**
		Checks the header to make sure it is a reasonable value.
		Only restraint on the header is that it must be 4 bytes.
		@tparam FwdIter Forward iterator
		@param [in] first Iterator to the first character in the header
		@param [in] last  Iterator past the last character in the header
		@return True if the header is valid, false otherwise
		*/
		template <typename FwdIter>
		bool validate_header(FwdIter first, FwdIter last) const
		{
			return std::distance(first, last) == header_size();
		}

		/**
		Serializes the passed in string into a message ready to be sent.
		@param [in] send_msg string to send
		@return Serialized message
		*/
		std::vector<char> serialize(const send_t& send_msg) const
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

		/**
		Deserializes a string from the message body.
		@tparam FwdIter Forward iterator
		@param [in] first Iterator to the first character in the body
		@param [in] last  Iterator past the last character in the body
		@return Deserialized string
		*/
		template <typename FwdIter>
		recv_t deserialize(FwdIter first, FwdIter last) const
		{
			return recv_t(first, last);
		}
	};
}