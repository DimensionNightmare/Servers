@echo off
cd /d %~dp0
start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=1 ip=127.0.0.1 port=1270
start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=2 port=1213 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=3 ip=0.0.0.0 port=1212 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=4 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=5 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
@REM start .\Out\Build\Debug\DimensionNightmareServer.exe svrType=6 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
