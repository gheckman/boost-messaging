#pragma once

#include <memory>

#include "tcp_reader.h"
#include "udp_reader.h"
#include "writer.h"

namespace boost_messaging
{
	template <typename TComm>
	struct reader_selector
	{
		using type =
			std::conditional<
			std::is_same<typename TComm::protocol_t, ip::tcp>::value,
			tcp_reader<TComm>,
			udp_reader<TComm>>;
	};

	template <typename TProtocol, typename TSerializer, typename THandler>
	class comm : public std::enable_shared_from_this<comm<TProtocol, TSerializer, THandler>>
	{
	public:
		typedef comm<TProtocol, TSerializer, THandler> this_t;
		typedef TProtocol protocol_t;
		typedef TSerializer serializer_t;
		typedef THandler handler_t;
		typedef typename protocol_t::socket socket_t;
		typedef tcp_reader<this_t> reader_t;
		typedef writer<this_t> writer_t;
		typedef typename TSerializer::send_t send_t;

		comm(io_service& io_service, socket_t&& socket) :
			io_service_(io_service),
			socket_(std::move(socket)),
			serializer_(),
			handler_(),
			reader_(*this),
			writer_(*this)
		{}

		inline void read() { reader_.read(); }
		inline void write(const send_t& send_msg) { writer_.write(send_msg); }

		inline io_service& get_io_service() { return io_service_; }
		inline socket_t& socket() { return socket_; }
		inline serializer_t& serializer() { return serializer_; }
		inline handler_t& handler() { return handler_; }

	private:
		io_service& io_service_;
		socket_t socket_;
		serializer_t serializer_;
		handler_t handler_;
		reader_t reader_;
		writer_t writer_;
	};
}