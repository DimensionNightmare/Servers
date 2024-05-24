#!/bin/bash
cd $(dirname $0)

tmux new-window -n svrType=1 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=1 port=1270"
tmux new-window -n svrType=2 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=2 port=1213 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270"
tmux new-window -n svrType=3 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=3 port=1212 byCtl=1 ctlIp=127.0.0.1 ctlPort=1270"
tmux new-window -n svrType=4 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=4 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213"
tmux new-window -n svrType=5 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=5 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213"
tmux new-window -n svrType=6 "./out/build/Debug_linux_llvm/DimensionNightmareServer svrType=6 byCtl=1 ctlIp=127.0.0.1 ctlPort=1213"
