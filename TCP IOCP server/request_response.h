#pragma once

interface app_state_machine
{
	// Returns true if processing is OK, false if the connection should be shutdown.
	virtual void reset() = 0;
	virtual void on_IO_complete(class connection& connection, OVERLAPPED_plus& overlapped, char* buffer, size_t bytes) = 0;
};

unsigned long long number_of_responses_sent_OK();
unsigned long long number_of_failed_sends();

enum class state_type { Start=50, Accepted=51, Finished=52 };

class sample_app_state_machine : public app_state_machine
{
public:
	sample_app_state_machine();

	static void initialise();
	void reset() override;

	void on_IO_complete(class connection& connection, OVERLAPPED_plus& overlapped, char* buffer, size_t bytes) override;

	void on_acceptex_complete(class connection& connection, OVERLAPPED_plus& overlapped, char* buffer, size_t bytes);
	void on_send_complete(class connection& connection, OVERLAPPED_plus& overlapped, char* buffer, size_t bytes);
	void on_finished(class connection& connection, OVERLAPPED_plus& overlapped);

private:
	std::mutex mtx;
	state_type state;
};
