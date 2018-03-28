#include "stdafx.h"
#include "../common/socket.h"
#include "ticker_source.h"
#include "timings.h"

using namespace std::chrono;

void timed_simulated_price_generation(ticker_source& source)
{
	int no_of_simulated_prices = 50000;
	system_clock::time_point before = high_resolution_clock::now();

	ticker* ticker = nullptr;
	for(int i=0; i<no_of_simulated_prices; ++i)
		source.get_random_simulated_price(ticker);
	
	system_clock::time_point after = high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time_taken = (after - before);
	std::cout << no_of_simulated_prices << " took " << time_taken.count() << " milliseconds to generate.\n";
}

void timed_message_generation(udp_socket<>& socket, socket_address& address, ticker_source& source)
{
	int no_of_simulated_prices = 50000;
	system_clock::time_point before = high_resolution_clock::now();

	ticker* ticker = nullptr;
	for(int i=0; i<no_of_simulated_prices; ++i)
	{
		source.get_random_simulated_price(ticker);
		socket.send_to(address, (const char*)ticker, sizeof(ticker));
	}

	system_clock::time_point after = high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time_taken = (after - before);
	std::cout << no_of_simulated_prices << " took " << time_taken.count() << " milliseconds to send.\n";
}

