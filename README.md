<p align="center">
  <img width=150 src="https://i.ibb.co/YZ7w1Dw/telescope.png" alt="telescope" align='center'>
</p>
<h1 align='center'>Telescope</h1>

### What is it?
Telescope is a high-performance remote desktop streaming solution that uses RDMA-capable hardware for low latency and CPU usage at full quality. It supports RoCE and InfiniBand capable network cards, as well as
software-based RDMA.

### A brief introduction to RDMA
RDMA (Remote Direct Memory Access) is a kernel bypass technology that allows peers
to directly read and write memory regions of their peers without any CPU involvement.
This allows for efficient, zero-copy, zero CPU usage data transfers. Internal testing
shows an approximate 90% CPU usage reduction for this particular use case.

### Why Telescope?
Telescope's main advantages are:
- High data rates with low CPU usage
- Fully uncompressed streaming with no blocking artifacts
- Place your host computer anywhere in your local network to manage its heat output
- Full 32-bit colour for professional-grade work

### What Telescope is not
Telescope is incredibly bandwidth-intensive and not designed as a solution for streaming
video over the Internet.  
Alternatives: [Parsec](https://parsec.app/) or [NoMachine](https://nomachine.com).  
Telescope is not designed for VM to host communications.  
Alternatives: [Looking Glass](https://github.com/gnif/LookingGlass/).

### Supported Configurations
The test version of Telescope supports Linux X11 clients/hosts.

### System Requirements
Please see the [System Requirements](https://github.com/beanfacts/telescope/wiki/System-Requirements) section of the wiki.

### Test environment
Telescope is built and tested on Fedora 34 and Ubuntu 21.04, with Mellanox ConnectX-3 NICs.  
Operating systems and hardware older/newer than these are likely to be supported, but they
have not been tested.

### Installing Telescope
Please refer to the [wiki](https://github.com/beanfacts/telescope/wiki/Installing-Telescope) for setup instructions.

### Copyright
&copy; 2021 Telescope Project Developers  
Tim Dettmar, Vorachat Somsuay, Jirapong Pansak, Tairo Kageyama, Kittidech Ditsuwan  
The license for this repository is AGPLv3- please see the LICENSE file for details.  
Exceptions may apply for specific first- and third-party libraries incorporated verbatim into this repository, which are copyrighted by the original author. 

### License Notices
This project would not be possible without the use of great third-party libraries and code examples.  
We use several libraries to speed up Telescope development, and the list is continuously changing.  
For the most recent information, please refer to the [wiki section](https://github.com/beanfacts/telescope/wiki/Third-Party-Libraries).
