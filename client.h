/**
@file client.h
@author Gary Heckman
@brief Generic client based on boost asio.
*/

#pragma once

#include <string>
#include <type_traits>

#include <boost\asio.hpp>
#include <boost\system\error_code.hpp>

#include "comm.h"

using namespace boost::asio;

namespace boost_messaging
{
    namespace detail
    {
        /**
        Exposes functions for clients that use the tcp protocol.
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TSerializer, typename THandler>
        class client_tcp_interface
        {
        public:
            typedef tcp_comm<TSerializer, THandler> comm_t;

            /**
            Sets up options that make tcp socket communication go smoothly.
            @param [in, out] socket Socket owned by the client's communcation object
            */
            void set_socket_opts(ip::tcp::socket& socket)
            {
                // Disables Nagle's algorithm.
                socket.set_option(ip::tcp::no_delay(true));
            }

            /**
            Sets the remote endpoint that the communcation object needs for udp communcation.
            Does nothing. This isn't needed for tcp. Only exists to fulfil an interface.
            @param [in, out] session  Client's session
            @param [in]      endpoint Remote endpoint
            */
            void set_remote_endpoint(comm_t& session, const ip::tcp::endpoint& endpoint) { }
        };

        /**
        Exposes functions for clients that use the udp protocol.
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TSerializer, typename THandler>
        class client_udp_interface
        {
        public:
            typedef udp_comm<TSerializer, THandler> comm_t;

            /**
            Sets up options that make udp socket communication go smoothly.
            Does nothing currently. Only exists to fulfil an interface.
            @param [in, out] socket Socket owned by the client's communcation object
            */
            void set_socket_opts(ip::udp::socket& socket) { }

            /**
            Sets the remote endpoint that the communcation object needs for udp communcation.
            @param [in, out] session  Client's session
            @param [in]      endpoint Remote endpoint
            */
            void set_remote_endpoint(comm_t& session, const ip::udp::endpoint& endpoint)
            {
                session.set_remote_endpoint(endpoint);
            }
        };

        /**
        Switches between client interfaces that use different communicaton protocols.
        @tparam TProtocol   Either boost::asio::ip::tcp or boost::asio::ip::udp
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TProtocol, typename TSerializer, typename THandler>
        struct client_interface_selector
        {
            using type =
                typename std::conditional<
                std::is_same<TProtocol, ip::tcp>::value,
                client_tcp_interface<TSerializer, THandler>,
                client_udp_interface<TSerializer, THandler>>::type;
        };
    }

    /**
    Generic client based on boost asio.
    @tparam TProtocol   Either boost::asio::ip::tcp or boost::asio::ip::udp
    @tparam TSerializer Type that fulfils the Serializer concept
    @tparam THandler    Type that fulfils the Handler concept
    */
	template <typename TProtocol, typename TSerializer, typename THandler>
	class client
	{
	public:
		typedef typename TProtocol::resolver resolver_t;
		typedef typename TProtocol::resolver::query query_t;
		typedef typename TProtocol::resolver::iterator iterator_t;
		typedef typename TSerializer::send_t send_t;
		typedef typename detail::comm_selector<TProtocol, TSerializer, THandler>::type comm_t;
		typedef typename detail::client_interface_selector<TProtocol, TSerializer, THandler>::type interface_t;

        /**
        Constructor.
        @param [in, out] io_service Facilitates async operations 
        @param [in]      host       IP or hostname of the server 
        @param [in]      port       Port number or port protocol name
        */
		client(io_service& io_service, const std::string& host, const std::string& port) :
			host_(host),
			port_(port),
			io_service_(io_service),
			session_(std::make_shared<comm_t>(io_service, TProtocol::socket(io_service))),
            connected_(false)
		{
            auto callback = [this](const boost::system::error_code& error) { error_callback(error); };
            session_->set_error_callback(callback);
			try_connect();
		}

        /**
        Writes a message to the connected server.
        @param [in] send_msg Message to be sent
        */
		void write(const send_t& send_msg)
		{
			session_->write(send_msg);
		}

        /**
        Closes the socket on the io_service thread.
        */
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
        bool connected_;

        /**
        Closes the socket.
        Must be run on the io_service thread.
        */
		void do_close()
		{
			session_->socket().close();
		}

        /**
        Tries to connect to the server.
        */
		void try_connect()
		{
			resolver_t resolver(io_service_);
			query_t query(host_, port_);
			iterator_t endpoint_iterator = resolver.resolve(query);

			auto callback = [this](const boost::system::error_code& error, iterator_t it) { connect(error, it); };
			boost::asio::async_connect(session_->socket(), endpoint_iterator, callback);
		}

        /**
        Processes an attempt at a connection.
        If successful, sets up socket options and starts reading.
        Otherwise tries to connect again.
        @param [in] error Error status of connection
        @param [in] it    Iterator to zero or more valid endpoints
        */
		void connect(const boost::system::error_code& error, iterator_t it)
		{
			if (!error)
			{
                connected_ = true;
				interface_.set_socket_opts(session_->socket());
				interface_.set_remote_endpoint(*session_, *it);
				session_->read();
			}
			else
			{
				if (connected_)
					error_print(error);
                connected_ = false;
				try_connect();
			}
		}

        /**
        Function that is called when the session experiences an error.
        @param [in] error Error status of communication
        */
        void error_callback(const boost::system::error_code& error)
        {
            error_print(error);
            close();
            try_connect();
        }

        /**
        Formats and prints the error code.
        @param [in] error Error status of communication
        */
        void error_print(const boost::system::error_code& error)
        {
            std::cout << error.message() << std::endl;
        }
	};
}
