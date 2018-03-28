#define Use_send
#define Use_recv

#include "targetver.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <queue>
#include <assert.h>
#include <memory>
#include "CppUnitTest.h"

#define Log_error(s) std::cerr << "Thread:" << std::this_thread::get_id() << " " << s << "\n"
#define Log_info(s)
#define Log_info2(s)

