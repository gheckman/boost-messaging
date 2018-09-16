#pragma once

#include <iostream>

template <typename T>
class print_handler
{
public:
	void handle(const T& t)
	{
		std::cout << t << std::endl;
	}
};