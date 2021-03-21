#!/bin/sh

#do a full upgrade of the OS
echo Update
sudo apt update
sudo apt -y full-upgrade
# RDMA libraries
echo Installing RDMA stuff

sudo apt install -y rdma-core
sudo apt install -y rdmacm-utils
sudo apt install -y librdmacm-dev

# install perftest a rdma benchmarking library
echo Installing perftest

sudo apt install -y perftest

# install X lib required for interfacing with X server
echo Installing xlib

sudo apt install -y libx11-dev

# install XCB libraries used to capture mouse and keyboard
echo Installing XCB Libs

sudo apt-get install -y xcb-proto
sudo apt-get install -y libxcb-xfixes0-dev

# X test library is used to move mouse
echo Installing Xtest
sudo apt install -y libxtst-dev
