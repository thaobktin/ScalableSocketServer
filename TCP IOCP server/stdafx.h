#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define Use_ReadFile
#define Use_WriteFile
//#define Use_WSARecv
//#define Use_WSASend

#ifdef HEAP_PER_THREAD
#define DEDICATED_HEAP__NO_LOCKING
#endif
#define NO_TRACING

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mswsock.h>
#include <Windows.h>
#include <process.h>
#include <memory>
#include <tchar.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <limits>
#include <chrono>
#include <boost/program_options.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <new>
#include <memory>

//#define SafeIncrement(x)
#define SafeIncrement(x) InterlockedIncrement(x)  // potential performance impact.
