@echo off
cd /d %~dp0

setlocal enabledelayedexpansion

set "server=%1"

if "%server%"=="" (
	@REM taskkill /f /im DimensionNightmareServer.exe
	call :control
	call :global
	call :auth
	call :gate
	call :database
	call :logic
	exit /b
)

call :%server%

:control
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=1 port=1270
goto :eof

:global
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=2 port=1213 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
goto :eof

:auth
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=3 port=1212 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
goto :eof

:gate
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=4 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof

:database
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=5 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof

:logic
start "" .\out\Build\Debug\DimensionNightmareServer.exe svrType=6 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
goto :eof
