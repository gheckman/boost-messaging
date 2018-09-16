#pragma once

template <typename TComm>
class udp_reader
{};

// BUFFER_SIZE = 0x4000

// read -> resize(BUFFER_SIZE), read_msg

// read_msg -> body_begin = buffer.begin() + header_size, validate header, get body, deserialize,  handle(message), async_read(read_body)

// set_remote_endpoint