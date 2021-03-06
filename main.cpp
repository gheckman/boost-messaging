#include <thread>

#include "client.h"
#include "server.h"
#include "print_handler.h"
#include "string_serializer.h"

using namespace boost::asio;
using namespace boost_messaging;
using namespace std::chrono_literals;

int main()
{
	typedef ip::tcp protocol_t;

	io_service io_service;
	io_service::work work(io_service);

	std::thread io_thread([&] { io_service.run(); });

    std::string type;
    std::getline(std::cin, type);

    if (type[0] == 's')
    {
        server<protocol_t, string_serializer, print_handler<std::string>> s1(io_service, protocol_t::endpoint(protocol_t::v4(), 12345));

        std::string to_send;
        while (std::getline(std::cin, to_send))
        {
            if (to_send == "quit")
                break;
        }
    }
    else
    {
	    client<protocol_t, string_serializer, print_handler<std::string>> c1(io_service, "127.0.0.1", "12345");

        std::string to_send;
        while (std::getline(std::cin, to_send))
        {
            if (to_send == "quit")
                break;

            c1.write("[client 1] " + to_send);
        }
    }

	io_service.stop();
	io_thread.join();
}