#include "stdafx.h"
#include "ticker_source.h"

using std::string;
using std::vector;
using std::runtime_error;

void split(string s, char delim, vector<string>& tokens)
{
	string token;
	for_each(s.begin(), s.end(), [&](char c) 
	{
		if(c != delim)
			token += c;
		else 
		{
			if(token.length())
				tokens.push_back(token);
			token.clear();
		}
	});
	if (token.length()) 
		tokens.push_back(token);
}

bool ticker_source::load(const char* filename)
{
	std::ifstream f;
	f.open(filename);
	if(!f.is_open())
		return false;
	int cnt = 0;
	while(!f.eof())
	{
		string line;
		std::getline(f, line);

		vector<string> tokens;
		split(line, ',', tokens);
		if(cnt++ != 0 && tokens.size() == 7)
		{
			struct ticker ticker;
				
			::strncpy(ticker.symbol, tokens[0].c_str(), sizeof(ticker.symbol));
			ticker.buy = stod(tokens[3]);
			ticker.sell = stod(tokens[4]);

			tickers.push_back(ticker);
		}
	}
	f.close();
	return true;
}

void ticker_source::get_random_simulated_price(struct ticker*& ticker)
{
	// get random symbol
	double no_of_symbols = tickers.size();
	if(no_of_symbols == 0)
		throw runtime_error("Ticker source needs to be loaded before get_random_simulated_price() is called");

#pragma warning(disable: 4244)
#undef max

	static double k1 = no_of_symbols/((double)RAND_MAX);
	int index = std::numeric_limits<int>::max();
	while(index >= no_of_symbols)
	{
		index = floor(k1*rand());
	}
	ticker = &tickers[index];

	static double k2 = 0.01/((double)RAND_MAX);
	double factor = (1.0 - (k2*rand() - 0.005));
	ticker->buy *= factor;
	ticker->sell *= factor;
}

