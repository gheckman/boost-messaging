#pragma once

#include <queue>
#include <vector>

#include <boost\asio.hpp>
#include <boost\system\error_code.hpp>

using namespace boost::asio;

template <typename TComm>
class writer
{
public:
	typedef typename TComm comm_t;
	typedef typename TComm::serializer_t serializer_t;
	typedef typename serializer_t::send_t send_t;

	writer(comm_t& owner) :
		owner_(owner),
		queue_()
	{}

	void write(const send_t& message)
	{
		current_msg_ = message;
		owner_.get_io_service().post([this] { enter_write_loop(); });
	}

private:
	comm_t& owner_;
	std::queue<std::vector<char>> queue_;
	send_t current_msg_;

	void enter_write_loop()
	{
		auto writing = !queue_.empty();
		queue_.push(owner_.serializer().serialize(current_msg_));
		if (!writing)
		{
			auto callback = [this, sp = owner_.shared_from_this()](const boost::system::error_code& error, size_t) { write_loop(error); };
			async_write(owner_.socket(), boost::asio::buffer(queue_.front()), callback);
		}
	}

	void write_loop(const boost::system::error_code& error)
	{
		if (!error)
		{
			queue_.pop();
			if (!queue_.empty())
			{
				auto callback = [this, sp = owner_.shared_from_this()](const boost::system::error_code& error, size_t) { write_loop(error); };
				async_write(owner_.socket(), boost::asio::buffer(queue_.front()), callback);
			}
		}
		//else
		//    error_callback
	}
};