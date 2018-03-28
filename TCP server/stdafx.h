#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define Use_recv
#define Use_send
#define NO_TRACING

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mswsock.h>
#include <tchar.h>
#include <cassert>
#include <fstream>
#include <istream>
#include <iostream>
#include <vector>
#include <list>
#include <boost/program_options.hpp>
#include <thread>
#include <queue>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

#define SafeIncrement(x)
//#define SafeIncrement(x) InterlockedIncrement(x)


