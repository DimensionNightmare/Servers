@echo off
cd /d %~dp0
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=1", "ip=127.0.0.1", "port=1270"
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=2", "port=1213", "byCtl=1", "ctlIp=127.0.0.1", "ctlPort=1270"
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=3", "ip=0.0.0.0", "port=1212", "byCtl=1", "ctlIp=127.0.0.1", "ctlPort=1270"
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=4", "byCtl=1", "ctlIp=127.0.0.1", "ctlPort=1213"
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=5", "byCtl=1", "ctlIp=127.0.0.1", "ctlPort=1213"
powershell -Command Start-Process .\Out\Build\Debug\DimensionNightmareServer.exe -ArgumentList "svrType=6", "byCtl=1", "ctlIp=127.0.0.1", "ctlPort=1213"