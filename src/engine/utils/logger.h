#ifndef __UTILS_LOGGER_HEADER__
#define __UTILS_LOGGER_HEADER__

#include <iostream>

#define LOG_TRACE(logger, MSG) std::cout << "TRAC: " << MSG << std::endl;
#define LOG_DEBUG(logger, MSG) std::cout << "DEBG: " << MSG << std::endl;
#define LOG_INFO(logger, MSG)  std::cout << "INFO: " << MSG << std::endl;
#define LOG_WARN(logger, MSG)  std::cout << "WARN: " << MSG << std::endl;
#define LOG_ERROR(logger, MSG) std::cout << "EROR: " << MSG << std::endl;
#define LOG_FATAL(logger, MSG) std::cout << "FATL: " << MSG << std::endl;

#endif