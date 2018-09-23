#pragma once

#include <functional>
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
        class tcp_comm : public std::enable_shared_from_this<tcp_comm<TSerializer, THandler>>
        {
        public:
            typedef ip::tcp::socket socket_t;
            typedef typename TSerializer::send_t send_t;
            typedef typename TSerializer::recv_t recv_t;
            typedef typename std::function<void(const boost::system::error_code& error)> error_callback_t;

            tcp_comm(io_service& io_service, socket_t&& socket) :
                io_service_(io_service),
                socket_(std::move(socket)),
                serializer_(),
                handler_(),
                error_callback_(nullptr)
            {}

            void read()
            {
                auto header_size = serializer_.header_size();
                read_buffer_.resize(header_size);
                auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { read_header(error); };
                async_read(socket_, boost::asio::buffer(read_buffer_), callback);
            }

            void write(const send_t& message)
            {
                io_service_.post([this, message] { enter_write_loop(message); });
            }

            inline socket_t& socket() { return socket_; }

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
            error_callback_t error_callback_;

            void read_header(const boost::system::error_code& error)
            {
                if (!error)
                {
                    auto header_is_valid = serializer_.validate_header(read_buffer_.begin(), read_buffer_.end());
                    // TODO [1] do something if header isn't valid
                    auto body_size = serializer_.body_size(read_buffer_.begin(), read_buffer_.end());
                    read_buffer_.resize(body_size);
                    auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { read_body(error); };
                    async_read(socket_, boost::asio::buffer(read_buffer_), callback);
                }
                else if (error_callback_)
                    error_callback_(error);
            }

            void read_body(const boost::system::error_code& error)
            {
                if (!error)
                {
                    auto mesage = serializer_.deserialize(read_buffer_.begin(), read_buffer_.end());
                    handler_.handle(mesage);

                    auto header_size = serializer_.header_size();
                    read_buffer_.resize(header_size);
                    auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { read_header(error); };
                    async_read(socket_, boost::asio::buffer(read_buffer_), callback);
                }
                else if (error_callback_)
                    error_callback_(error);
            }

            void enter_write_loop(const send_t& message)
            {
                auto writing = !write_queue_.empty();
                write_queue_.push(serializer_.serialize(message));
                if (!writing)
                {
                    auto callback = [this, sp = this->shared_from_this()](const boost::system::error_code& error, size_t) { write_loop(error); };
                    async_write(socket_, boost::asio::buffer(write_queue_.front()), callback);
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
                        async_write(socket_, boost::asio::buffer(write_queue_.front()), callback);
                    }
                }
                else if (error_callback_)
                    error_callback_(error);
            }
        };
    }
}