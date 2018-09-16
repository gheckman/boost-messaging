#pragma once

#include <string>
#include <type_traits>

#include <boost\asio.hpp>
#include <boost\system\error_code.hpp>

#include "comm.h"

using namespace boost::asio;

namespace boost_messaging
{
	template <typename TSerializer, typename THandler>
	class client_tcp_interface
	{
	public:
		typedef tcp_comm<TSerializer, THandler> comm_t;

		void set_socket_opts(ip::tcp::socket& socket)
		{
			socket.set_option(ip::tcp::no_delay(true));
		}

		void set_remote_endpoint(comm_t& session, const ip::tcp::endpoint& endpoint) {}

	};

	template <typename TSerializer, typename THandler>
	class client_udp_interface
	{
	public:
		typedef udp_comm<TSerializer, THandler> comm_t;

		void set_socket_opts(ip::udp::socket& socket) {}

		void set_remote_endpoint(comm_t& session, const ip::udp::endpoint& endpoint)
		{
			session.set_remote_endpoint(endpoint);
		}
	};

	template <typename TProtocol, typename TSerializer, typename THandler>
	struct client_interface_selector
	{
		using type =
			typename std::conditional<
				std::is_same<TProtocol, ip::tcp>::value,
				client_tcp_interface<TSerializer, THandler>,
				client_udp_interface<TSerializer, THandler>>::type;
	};

	template <typename TProtocol, typename TSerializer, typename THandler>
	class client
	{
	public:
		typedef typename TProtocol::resolver resolver_t;
		typedef typename TProtocol::resolver::query query_t;
		typedef typename TProtocol::resolver::iterator iterator_t;
		typedef typename TSerializer::send_t send_t;
		typedef typename comm_selector<TProtocol, TSerializer, THandler>::type comm_t;
		typedef typename client_interface_selector<TProtocol, TSerializer, THandler>::type interface_t;

		client(io_service& io_service, const std::string& host, const std::string& port) :
			host_(host),
			port_(port),
			io_service_(io_service),
			session_(std::make_shared<comm_t>(io_service, TProtocol::socket(io_service)))
		{
			// set_error_callback
			try_connect();
		}

		void write(const send_t& send_msg)
		{
			session_->write(send_msg);
		}

		void close()
		{
			io_service_.post([this] { do_close(); });
		}

	private:
		std::string host_;
		std::string port_;
		io_service& io_service_;
		std::shared_ptr<comm_t> session_;
		interface_t interface_;

		void do_close()
		{
			session_->socket.close();
		}

		void try_connect()
		{
			resolver_t resolver(io_service_);
			query_t query(host_, port_);
			iterator_t endpoint_iterator = resolver.resolve(query);

			auto callback = [this](const boost::system::error_code& error, iterator_t it) { connect(error, it); };
			boost::asio::async_connect(session_->socket(), endpoint_iterator, callback);
		}

		void connect(const boost::system::error_code& error, iterator_t it)
		{
			if (!error)
			{
				interface_.set_socket_opts(session_->socket());
				interface_.set_remote_endpoint(*session_, *it);
				session_->read();
			}
			else
			{
				//if (was_connected)
				//	error_print(error);
				try_connect();
			}
		}

		// error_callback -> error_print, close, try_connect
	};
}
