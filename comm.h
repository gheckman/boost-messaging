#pragma once

#include "tcp_comm.h"
#include "udp_comm.h"

namespace boost_messaging
{
	template <typename TProtocol, typename TSerializer, typename THandler>
	struct comm_selector
	{
		using type =
			typename std::conditional<
			std::is_same<TProtocol, ip::tcp>::value,
			tcp_comm<TSerializer, THandler>,
			udp_comm<TSerializer, THandler>>::type;
	};
}
