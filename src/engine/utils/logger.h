#ifndef __UTILS_LOGGER_HEADER__
#define __UTILS_LOGGER_HEADER__

#include <iostream>

#define LOG_TRACE(logger, MSG) do { std::cout << "TRAC: " << MSG << std::endl; } while(false)
#define LOG_DEBUG(logger, MSG) do { std::cout << "DEBG: " << MSG << std::endl; } while(false)
#define LOG_INFO(logger, MSG)  do { std::cout << "INFO: " << MSG << std::endl; } while(false)
#define LOG_WARN(logger, MSG)  do { std::cout << "WARN: " << MSG << std::endl; } while(false)
#define LOG_ERROR(logger, MSG) do { std::cout << "EROR: " << MSG << std::endl; } while(false)
#define LOG_FATAL(logger, MSG) do { std::cout << "FATL: " << MSG << std::endl; } while(false)

#endif