# telescope

## What is telescope?
Telescope is a RDMA based Desktop streaming application

## What is RDMA?
RDMA stands for Remote Direct Memory Access. Using RDMA allows a 
remote client to **directly** write to the memory of the host system  
Remote Direct Memory Access (RDMA) allows
computers in a network to exchange data in main memory without
involving the processor, cache or operating system of either computer.
## Why RDMA?
Like locally based Direct Memory Access (DMA), RDMA improves 
throughput and performance because it frees up resources.

## Telescope vs Other Solutions
### Advantages:
1. Higher Datarates
2. No Compression
3. Your Main PC can be far away from you
4. Full 24-bit color
### Disadvantages:
1. Higher cost than using compressed alternatives such as parsec

## Hardware Requirements

### Bandwidth
Telescope streams uncompressed 24-bit-per-pixel 1080p video stream by default.
The data rates of this averages around 3 Gbps therefore you 
will need a 10 Gbps NIC. The stream can be pushed to higher 
resolutions with better networking

### RDMA and RoCE
Telescope relies on the RDMA protocol which can either be used over
native infiniband or can be used with RoCE therefore the NIC used must
also be able to support either of the protocols


## Development environment:
### OS:
PopOs 20.10

### NIC
2 * [Mellenox Connect-X3](https://www.lazada.co.th/products/pulled-mellanox-connectx-3-cx341a-mcx341a-10gbe-pci-e-network-card-i1816010097-s5439296495.html?spm=a2o4m.searchlist.list.33.2dd271e4qex5fV&search=1)

### Interconnect
Generic SFP+ cable

### Server System:

### Client System