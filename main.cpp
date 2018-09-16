#include "client.h"
#include "server.h"
#include "print_handler.h"
#include "string_serializer.h"

using namespace boost::asio;

using namespace boost_messaging;


int main()
{
	boost::asio::io_service io_service;
	boost::asio::io_service::work work(io_service);

	std::thread io_thread([&] { io_service.run(); });

	server<string_serializer, print_handler<std::string>> s1(io_service, ip::tcp::endpoint(ip::tcp::v4(), 12345));
	client<string_serializer, print_handler<std::string>> c1(io_service, "127.0.0.1", "12345");
	//client<string_serializer, print_handler<std::string>> c2(io_service, "127.0.0.1", "12345");

	std::string to_send;
	while (std::getline(std::cin, to_send))
	{
		if (to_send == "quit")
			break;

		c1.write("[client 1] " + to_send);
		//c2.write("[client 2] " + to_send);
	}

	io_service.stop();
	io_thread.join();
}