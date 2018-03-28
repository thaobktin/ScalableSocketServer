#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define Use_recv
#define Use_send
#define NO_TRACING

#define _CRT_SECURE_NO_WARNINGS

// HEAP OPTIONS
//#define HEAP__USE_STD_VECTOR
//#define HEAP__USE_VirtualAlloc
//#define DEDICATED_HEAP__NO_LOCKING

#include "targetver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mswsock.h>
#include <stdio.h>
#include <tchar.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <limits>
#include <boost/program_options.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
