<p align="center">
  <img width=150 src="https://i.ibb.co/YZ7w1Dw/telescope.png" alt="telescope" align='center'>
</p>
<h1 align='center'>Telescope</h1>
## What is it?
Telescope is a high-performance remote desktop streaming solution that uses RDMA-capable hardware for low latency and CPU usage at full quality.  
It supports RDMA capable Ethernet (RoCE) or InfiniBand (RDMAoIB) networking hardware, as well as software RDMA.

## A brief introduction to RDMA
Remote Direct Memory Access is a technology that allows machines to directly read 
and write memory regions of their peers. This allows for a CPU and kernel bypass, allowing
for zero-copy, zero CPU usage memory copies, allowing large amounts of data to be moved with little
transmission overhead.

## Why Telescope?
Telescope's main advantages are
- High data rates with low CPU usage
- Fully uncompressed streaming with no blocking artifacts
- Place your host computer anywhere in your local network to manage its heat output
- Full 24-bit colour for professional-grade work

If you are looking for a solution to stream your remote desktop session over the Internet, this repository does not aim to solve that issue.
We recommend checking out [Parsec](https://parsec.app/) or [NoMachine](https://nomachine.com) if this is what you need.

## Hardware Requirements

### Bandwidth
Telescope stream resolution is determined by your monitor resolution and refresh rate of your X display.
By default, it streams at 24bpp without any chroma subsampling - For a 1080p/60Hz video stream, you will require approximately 3.1 Gbps of available network throughput.  
To calculate your bandwidth requirement, the formula is:  
`BW (Mbps) = <width> * <height> * <bit depth> * <framerate> / 1048576`.

## Test environment
- OS: Pop!\_OS 20.10, Debian 10.0, Ubuntu 18.04
- NIC: Mellanox ConnectX-3 10GbE NIC w/ RDMA

## Installing Telescope

Please refer to the [wiki](https://github.com/beanfacts/telescope/wiki) for setup instructions.

## Copyright
&copy; 2021 Telescope Project Developers  
(Tim Dettmar, Vorachat Somsuay, Jirapong Pansak, Tairo Kageyama, Kittidech Ditsuwan)  
This repository is licensed under the GNU Affero General Public License, Version 3
with the distribution terms specified in the license.

## License Notices
RDMA baseline test code is based on the documentation and tutorials provided by Mellanox and Tarick Bedeir, licensed under the MIT license.
