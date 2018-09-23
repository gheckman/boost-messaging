/**
@file server.h
@author Gary Heckman
@brief Generic server based on boost asio.
*/

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
    namespace detail
    {
        /**
        Exposes functions for servers that use the tcp protocol.
        This server type generates multiple sessions: one for each connection.
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TSerializer, typename THandler>
        class server_tcp_interface
        {
        public:
            typedef tcp_comm<TSerializer, THandler> comm_t;

            /**
            Constructor.
            @param [in, out] io_service Facilitates async operations 
            @param [in]      endpoint   Endpoint used to accept connections
            */
            server_tcp_interface(io_service& io_service, const ip::tcp::endpoint& endpoint) :
                io_service_(io_service),
                acceptor_(io_service, endpoint)
            { }

            /**
            Asynchronously waits for new incoming connections.
            */
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
            Erases expired sessions.
            */
            void clean()
            {
                auto is_expired = [](std::weak_ptr<comm_t> session) { return session.expired(); };
                auto end_it = std::remove(sessions_.begin(), sessions_.end(), is_expired);
                sessions_.erase(end_it, sessions_.end());
            }

            /**
            Writes to each active session.
            Removes expired sessions.
            @return True if any message was written, false otherwise.
            */
            bool write()
            {
                bool did_write = false;
                clean();

                for (auto session : sessions_)
                {
                    // Must still check if the session expired since cleaning.
                    // If the session is still alive, write to it
                    if (auto sp = session.lock())
                    {
                        sp->write();
                        did_write = true;
                    }
                }

                return did_write;
            }

            /**
            Writes to the session that matches the endpoint.
            Removes expired sessions.
            @param [in] endpoint Remote endpoint to write to
            @return True if the message was written, false otherwise.
            */
            bool write(const ip::tcp::endpoint& endpoint)
            {
                bool did_write = false;
                clean();

                auto lock_compare_endpoints = [&](std::weak_ptr<comm_t> session) { auto sp = session.lock(); return sp && sp->socket().remote_endpoint() == endpoint; };
                auto it = std::find_if(sessions_.begin(), sessions_.end(), lock_compare_endpoints);

                if (it != sessions_.end())
                {
                    // Must still check if the session expired since cleaning.
                    // If the session is still alive, write to it
                    if (auto sp = it->lock())
                    {
                        sp->write();
                        did_write = true;
                    }
                }

                return did_write;
            }

            /**
            Starts reading from the new connection, then accepts more connections.
            @param [in, out] session 
            @param [in]      error   
            */
            void handle_accept(std::shared_ptr<comm_t> session, const boost::system::error_code& error)
            {
                sessions_.emplace_back(session);
                session->read();
                start_accept();
            }
        };

        /**
        Exposes functions for servers that use the udp protocol.
        This server type has a single, connectionless session.
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TSerializer, typename THandler>
        class server_udp_interface
        {
        public:
            typedef udp_comm<TSerializer, THandler> comm_t;

            /**
            Constructor.
            @param [in, out] io_service Facilitates async operations
            @param [in]      endpoint   Endpoint of the server session
            */
            server_udp_interface(io_service& io_service, const ip::udp::endpoint& endpoint) :
                session_(std::make_shared<comm_t>(io_service, ip::udp::socket(io_service, endpoint)))
            {
                port_ = endpoint.port();         
            }

            /**
            Starts reading from the session.
            */
            void start_accept()
            {
                session_->read();
            }

            /**
            Broadcasts.
            @return True
            */
            bool write()
            {
                ip::udp::endpoint boradcast_endpoint(ip::address_v4::broadcast(), port_);
                session_->set_remote_endpoint(boradcast_endpoint);
                session_->write();
                return true;
            }

            /**
            Writes to the session.
            @return True
            */
            bool write(const ip::udp::endpoint& endpoint)
            {
                session_->set_remote_endpoint(endpoint);
                session_->write();
                return true;
            }

        private:
            uint16_t port_;
            std::shared_ptr<comm_t> session_;
        };

        /**
        Switches between server interfaces that use different communicaton protocols.
        @tparam TProtocol   Either boost::asio::ip::tcp or boost::asio::ip::udp
        @tparam TSerializer Type that fulfils the Serializer concept
        @tparam THandler    Type that fulfils the Handler concept
        */
        template <typename TProtocol, typename TSerializer, typename THandler>
        struct server_interface_selector
        {
            using type =
                typename std::conditional<
                std::is_same<TProtocol, ip::tcp>::value,
                server_tcp_interface<TSerializer, THandler>,
                server_udp_interface<TSerializer, THandler>>::type;
        };
    }

    /**
    Generic server based on boost asio.
    @tparam TProtocol   Either boost::asio::ip::tcp or boost::asio::ip::udp
    @tparam TSerializer Type that fulfils the Serializer concept
    @tparam THandler    Type that fulfils the Handler concept
    */
	template <typename TProtocol, typename TSerializer, typename THandler>
	class server
	{
	public:
		typedef typename TProtocol::endpoint endpoint_t;
		typedef typename detail::comm_selector<TProtocol, TSerializer, THandler>::type comm_t;
		typedef typename detail::server_interface_selector<TProtocol, TSerializer, THandler>::type interface_t;

        /**
        Constructor.
        @param [in, out] io_service Facilitates async operations
        @param [in]      host       IP or hostname of the server
        @param [in]      port       Port number or port protocol name
        */
		server(io_service& io_service, const endpoint_t& endpoint) :
			interface_(io_service, endpoint)
		{
			interface_.start_accept();
		}

        /**
        Writes to the session.
        */
        void write()
        {
            interface_.write();
        }

        /**
        Writes to a specific session based on endpoint.
        @param [in] endpoint Endpoint of the client
        */
        void write(const endpoint_t& endpoint)
        {
            interface_.write(endpoint);
        }

	private:
		typename interface_t interface_;
	};
}
