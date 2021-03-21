#!/bin/sh
echo Update
sudo apt update
sudo apt -y full-upgrade

echo Installing RDMA stuff

sudo apt install -y rdma-core
sudo apt install -y rdmacm-utils

echo Installing perftest

sudo apt install -y perftest

echo Installing xlib

sudo apt install -y libx11-dev

echo Installing XCB Libs

sudo apt-get install -y xcb-proto
sudo apt-get install -y libxcb-xfixes0-dev

echo Installing Xtest
sudo apt install -y libxtst-dev

echo Installing X11 library
sudo apt install -y libx11-dev
