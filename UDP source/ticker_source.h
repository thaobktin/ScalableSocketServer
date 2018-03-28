#pragma once

struct ticker
{
	int msg_ID;
	char symbol[4];
	double buy;
	double sell;
};

inline const char* ticker_symbol(const ticker& ticker)
{
	static char buf[8] {0,0,0,0,0,0,0,0};
	static_assert(sizeof(int) == 4, "code depends on sizeof(int) == 4");
	*(int*)buf = *(int*)(&ticker.symbol);
	return buf;
}

class ticker_source
{
public:
	bool load(const char* filename);

	void get_random_simulated_price(ticker*& ticker);
private:
	std::vector<ticker> tickers;
};

