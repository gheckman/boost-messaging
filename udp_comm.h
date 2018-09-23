#pragma once

#include <memory>
#include <queue>
#include <vector>

#include <boost/asio.hpp>

using namespace boost::asio;

namespace boost_messaging
{
    namespace detail
    {
        template <typename TSerializer, typename THandler>
        class udp_comm : public std::enable_shared_from_this<udp_comm<TSerializer, THandler>>
        {
        public:
            typedef ip::udp::socket socket_t;
            typedef typename TSerializer::send_t send_t;
            typedef typename TSerializer::recv_t recv_t;
            typedef typename std::function<void(const boost::system::error_code& error)> error_callback_t;

            const int BUFFER_SIZE = 0x4000;

            udp_comm(io_service& io_service, socket_t&& socket) :
                io_service_(io_service),
                socket_(std::move(socket)),
                serializer_(),
                handler_(),
                read_buffer_(BUFFER_SIZE)
            {}

            void read()
            {
                auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { read_msg(error); };
                socket_.async_receive_from(boost::asio::buffer(read_buffer_), endpoint_, callback);
            }

            void write(const send_t& message)
            {
                io_service_.post([this, message] { enter_write_loop(message); });
            }

            inline socket_t& socket() { return socket_; }

            inline void set_remote_endpoint(const ip::udp::endpoint& endpoint) { endpoint_ = endpoint; }

            void set_error_callback(const error_callback_t& error_callback)
            {
                error_callback_ = error_callback;
            }

        private:
            io_service & io_service_;
            socket_t socket_;
            TSerializer serializer_;
            THandler handler_;
            std::vector<char> read_buffer_;
            std::queue<std::vector<char>> write_queue_;
            ip::udp::endpoint endpoint_;
            error_callback_t error_callback_;

            void read_msg(const boost::system::error_code& error)
            {
                if (!error)
                {
                    auto header_size = serializer_.header_size();
                    auto body_begin = read_buffer_.begin() + header_size;
                    auto header_is_valid = serializer_.validate_header(read_buffer_.begin(), body_begin);
                    auto body_size = serializer_.body_size(read_buffer_.begin(), body_begin);
                    auto body_end = body_begin + body_size;
                    auto message = serializer_.deserialize(body_begin, body_end);
                    handler_.handle(message);
                    auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { read_msg(error); };
                    socket_.async_receive_from(boost::asio::buffer(read_buffer_), endpoint_, callback);
                }
                else
                    error_callback_(error);
            }

            void enter_write_loop(const send_t& message)
            {
                auto writing = !write_queue_.empty();
                write_queue_.push(serializer_.serialize(message));
                if (!writing)
                {
                    auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { write_loop(error); };
                    socket_.async_send_to(boost::asio::buffer(write_queue_.front()), endpoint_, callback);
                }
            }

            void write_loop(const boost::system::error_code& error)
            {
                if (!error)
                {
                    write_queue_.pop();
                    if (!write_queue_.empty())
                    {
                        auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { write_loop(error); };
                        socket_.async_send_to(boost::asio::buffer(write_queue_.front()), endpoint_, callback);
                    }
                }
                else
                    error_callback_(error);
            }
        };
    }
}