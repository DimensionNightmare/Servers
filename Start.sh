#!/bin/bash
cd $(dirname $0)

# out/build/LLVM/DimensionNightmareServer svrType=1 ip=127.0.0.1 port=1270
out/build/LLVM/DimensionNightmareServer svrType=2 port=1213 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
# out/build/LLVM/DimensionNightmareServer svrType=3 ip=0.0.0.0 port=1212 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270
# out/build/LLVM/DimensionNightmareServer svrType=4 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
# out/build/LLVM/DimensionNightmareServer svrType=5 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
# out/build/LLVM/DimensionNightmareServer svrType=6 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213
