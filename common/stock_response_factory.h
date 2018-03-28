#pragma once

class stock_response_factory
{
	static char* stock_response;
	static size_t stock_response_length;
	static std::string server_name;

	static void load_response_from_file(const std::string& filename, allocator_type& _allocator);
	static void update_server_name();
	static void update_request_no();
public:
	static void set_server_name(const char* _server_name);

	static void get_stock_response(const char*& response, size_t& response_length);

	static void increment_request_count();
	static unsigned long get_request_count();  // this is test code - 2^32 is enough

	static bool initialise(allocator_type& _allocator, const std::string& response_filename = "");
};

// Noddy class used to replace a fixed piece of text in an text block.
// The text to be replaced is located in the ctor by matching the pass-in pattern.
// Subsequent substitutions via replace do not look for the text again, just does the replacement.
class text_substitution
{
	char *first, *end;
	const char* pattern;
	size_t pattern_len;
	char* find();
public:
	char *ptr; // pointer to the start of the text to be replaced.
	text_substitution(char* first, char* last, const char* pattern);
	void replace(const char* replacement, char fill_char);
};

