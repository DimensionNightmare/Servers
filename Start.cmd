@echo off
cd /d %~dp0

setlocal enabledelayedexpansion

set "server=%1"

set "menu=.\out\Build\Debug"

set "menuType=%2"

if "%menuType%"=="R" (
	set "menu=.\out\Build\Release"
)

if "%server%"=="_" (
	@REM taskkill /f /im DimensionNightmareServer.exe
	call :_control
	call :_global
	call :_auth
	call :_gate
	call :_database
	call :_logic
	exit /b
)

call :%server%

exit /b

:_control
start "" %menu%\DimensionNightmareServer.exe svrType=1 port=1270
goto :eof

:_global
start "" %menu%\DimensionNightmareServer.exe svrType=2 port=1213 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
goto :eof

:_auth
start "" %menu%\DimensionNightmareServer.exe svrType=3 port=1212 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
goto :eof

:_gate
start "" %menu%\DimensionNightmareServer.exe svrType=4 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof

:_database
start "" %menu%\DimensionNightmareServer.exe svrType=5 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof

:_logic
start "" %menu%\DimensionNightmareServer.exe svrType=6 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof
