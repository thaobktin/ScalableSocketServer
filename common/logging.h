#pragma once
// Copyright (c) Shaun O'Kane. 2012
// We don't want to cause any performance issues so go for logger simplicity, but define 
// an interface that is sufficiently sophisticated that extra features can be re-implemented later 
// without changes to the hosting source code.

#define INFO_LEVEL_2

#define Log_display(source, msg) std::cout << #source << ",Thread:" << std::this_thread::get_id() << " " << msg << "\n"
#define Log_error(source, msg) std::cerr << #source << ",Thread:" << std::this_thread::get_id() << " " << msg << "\n"

#if defined(NO_TRACING)
#pragma message("No trace functionality is being compiled into the demo. Error logging is still enabled.")
#define Log_info(source, msg)
#define Log_info2(source, msg)

#elif defined(INFO_LEVEL_1)
#pragma message("Compiling log_info into demo")
#define Log_info(source, msg) std::cout << #source << ",Thread:" << std::this_thread::get_id() << " " << msg << "\n"
#define Log_info2(source, msg)

#elif defined(INFO_LEVEL_2)
#pragma message("Compiling log_info into demo")
#define Log_info(source, msg) std::cout << #source << ", Thread:" << std::this_thread::get_id() << " " << msg << "\n"
#pragma message("Compiling log_info2 into demo")
#define Log_info2(source, msg) std::cout << #source << ", Thread:" << std::this_thread::get_id() << " " << msg << "\n"

#else
#pragma message("Please choose logging level")
#pragma message("****************************************** STOP THE BUILD!!!")
#endif

