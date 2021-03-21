#!/bin/sh
echo Installing xlib

sudo apt install -y libx11-dev

echo Installing XCB Libs

sudo apt-get install -y xcb-proto
sudo apt-get install -y libxcb-xfixes0-dev

echo Installing Xtest
sudo apt install -y libxtst-dev
