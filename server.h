#pragma once

#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include <boost\asio.hpp>

#include "comm.h"

using namespace boost::asio;

namespace boost_messaging
{
	template <typename TSerializer, typename THandler>
	class server_tcp_interface
	{
	public:
		typedef comm<ip::tcp, TSerializer, THandler> comm_t;

		server_tcp_interface(io_service& io_service, const ip::tcp::endpoint& endpoint) :
			io_service_(io_service),
			acceptor_(io_service, endpoint)
		{ }

		void start_accept()
		{
			auto new_session = std::make_shared<comm_t>(io_service_, ip::tcp::socket(io_service_));
			auto callback = [this, new_session](const boost::system::error_code& error) { handle_accept(new_session, error); };
			acceptor_.async_accept(new_session->socket(), callback);
		}

	private:
		ip::tcp::acceptor acceptor_;
		io_service& io_service_;
		std::list<std::weak_ptr<comm_t>> sessions_;

		/**
		Writes to each active session.
		Removes expired sessions.
		*/
		void write()
		{
			for (auto it = sessions_.begin(); it != sessions_.end();)
			{
				if (auto session = it->lock())
				{
					session->write();
					++it;
				}
				else
					it = sessions_.erase(it);
			}
		}

		void handle_accept(std::shared_ptr<comm_t> session, const boost::system::error_code& error)
		{
			sessions_.emplace_back(session);
			session->read();
			start_accept();
		}
	};

	template <typename TSerializer, typename THandler>
	class server_udp_interface
	{
	public:
		typedef comm<ip::udp, TSerializer, THandler> comm_t;

		server_udp_interface(io_service& io_service, const ip::udp::endpoint& endpoint)
		{ }

		void start_accept()
		{
			session_.read();
		}

		void write()
		{
			session_.write();
		}
	private:
		comm_t session_;
	};

	//template <typename TProtocol>
	//struct server_selector
	//{
	//	using type =
	//		std::conditional<
	//		std::is_same<TProtocol, ip::tcp>::value,
	//		server_tcp_interface,
	//		server_udp_interface>;
	//};

	template <typename TSerializer, typename THandler>
	class server
	{
	public:
		typedef ip::tcp protocol_t;
		typedef comm<protocol_t, TSerializer, THandler> comm_t;
		typedef server_tcp_interface<TSerializer, THandler> interface_t;

		server(io_service& io_service, const protocol_t::endpoint& endpoint) :
			interface_(io_service, endpoint)
		{
			interface_.start_accept();
		}

		void write()
		{
			interface_.write();
		}

	private:
		typename interface_t interface_;
	};
}
