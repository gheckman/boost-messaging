#pragma once

#include <boost\asio.hpp>
#include <boost\system\error_code.hpp>

using namespace boost::asio;

template <typename TComm>
class tcp_reader
{
public:
	typedef typename TComm comm_t;
	typedef typename TComm::serializer_t serializer_t;
	typedef typename TComm::handler_t handler_t;
	typedef typename serializer_t::recv_t recv_t;

	tcp_reader(comm_t& owner) :
		owner_(owner),
		buffer_()
	{}

	void read()
	{
		auto header_size = owner_.serializer().header_size();
		buffer_.resize(header_size);
		auto callback = [this, sp = owner_.shared_from_this()](const boost::system::error_code& error, size_t) { read_header(error); };
		async_read(owner_.socket(), boost::asio::buffer(buffer_), callback);
	}

private:
	comm_t& owner_;
	std::vector<char> buffer_;

	void read_header(const boost::system::error_code& error)
	{
		if (!error)
		{
			auto header_is_valid = owner_.serializer().validate_header(buffer_);
			// TODO [1] do something if header isn't valid
			auto body_size = owner_.serializer().body_size(buffer_);
			buffer_.resize(body_size);
			auto callback = [this, sp = owner_.shared_from_this()](const boost::system::error_code& error, size_t) { read_body(error); };
			async_read(owner_.socket(), boost::asio::buffer(buffer_), callback);
		}
		//else
		//    error_callback
	}

	void read_body(const boost::system::error_code& error)
	{
		if (!error)
		{
			auto mesage = owner_.serializer().deserialize(buffer_);
			owner_.handler().handle(mesage);

			auto header_size = owner_.serializer().header_size();
			buffer_.resize(header_size);
			auto callback = [this, sp = owner_.shared_from_this()](const boost::system::error_code& error, size_t) { read_header(error); };
			async_read(owner_.socket(), boost::asio::buffer(buffer_), callback);
		}
		//else
		//    error_callback
	}
};