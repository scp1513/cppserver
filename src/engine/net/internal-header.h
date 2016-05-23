#ifndef __NET_INTERNAL_HEADER__
#define __NET_INTERNAL_HEADER__

// _WIN32_WINNT version constants
#define _WIN32_WINNT_NT4          0x0400 // Windows NT 4.0
#define _WIN32_WINNT_WIN2K        0x0500 // Windows 2000
#define _WIN32_WINNT_WINXP        0x0501 // Windows XP
#define _WIN32_WINNT_WS03         0x0502 // Windows Server 2003
#define _WIN32_WINNT_WIN6         0x0600 // Windows Vista
#define _WIN32_WINNT_VISTA        0x0600 // Windows Vista
#define _WIN32_WINNT_WS08         0x0600 // Windows Server 2008
#define _WIN32_WINNT_LONGHORN     0x0600 // Windows Vista
#define _WIN32_WINNT_WIN7         0x0601 // Windows 7
#define _WIN32_WINNT_WIN8         0x0602 // Windows 8
#define _WIN32_WINNT_WINBLUE      0x0603 // Windows 8.1
#define _WIN32_WINNT_WINTHRESHOLD 0x0A00 // Windows 10
#define _WIN32_WINNT_WIN10        0x0A00 // Windows 10

#define _WIN32_WINNT _WIN32_WINNT_WIN10

#define ASIO_STANDALONE
#define ASIO_HAS_STD_CHRONO

#define BOOST_DATE_TIME_NO_LIB
#define BOOST_REGEX_NO_LIB

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/deadline_timer.hpp>

#endif