@echo off

SET DIR_ASIO=..\src\3rd\asio-1.10.6

SET DIR_RAPIDJSON=..\src\3rd\rapidjson-1.0.2

SET DIR_MYSQL_INCLUDE=..\src\3rd\mysql-connector-c-6.1.6-win32\include
SET DIR_MYSQL_LIB=..\src\3rd\mysql-connector-c-6.1.6-win32\lib

cd ..
if not exist "proj_vc14" (md "proj_vc14")
cd proj_vc14
if exist "CMakeCache.txt" (del "CMakeCache.txt")
if exist "CMakeFiles" rd /S /Q "CMakeFiles"
cmake -G "Visual Studio 14 2015" --build ../src
ping /n 10 127.0 > NUL