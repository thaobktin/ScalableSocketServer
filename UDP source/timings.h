#pragma once

void timed_message_generation(udp_socket<>& socket, class socket_address& address, class ticker_source& source);
void timed_simulated_price_generation(class ticker_source& source);