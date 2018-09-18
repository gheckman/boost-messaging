/**
@author Gary Heckman
@file print_handler.h
@brief Simple print handler example
@detail
	Example handler for use with the boost messaging library. It fulfils the
	Handler concept. See README for more information.
*/

#pragma once

#include <iostream>

namespace boost_messaging
{
	/**
	Handles receiving messages.
	Simply prints them to standard out.
	@tparam T Any type that overloads the << operator
	*/
	template <typename T>
	class print_handler
	{
	public:
		/**
		Prints the message it recieves to standard out.
		*/
		void handle(const T& t)
		{
			std::cout << t << std::endl;
		}
	};
}