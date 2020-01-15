# What is it ?
It's a ring buffer implementation in C made for capturing standard Ethernet frames (1500 byte maximum) at 10 Gb/s. 

## So it's like pluggable `dd`?
Basically, yes. But it's already faster for use case described above.

## What chunk size is the best?
It's made to receive standard IEEE 802.3 frames (which are 1500 B maximum) and write to RAID0 of SSD drives (where highest speed is achieved with chunk size of several MiBs). I'm not interested in other use cases yet. The whole point of using a ring buffer is to combine Ethernet frames to bigger chunks.

## What are these `intrin_memcpy_xxx` functions?
Standard library `memcpy` is just a loop, copying data by chunks that don't exceed the biggest standard data type (64 bit). It's not the most efficient way on modern CPUs with AVX (which basically introduces 128/256/512 bit registers). My functions use a naive implementation of intrinsics but they are optimal for small chunks (under 10000 B). However, if you need to copy bigger chunks, search for `apex_kryptonite` (some guy wrote an ultimate intrinsics-based memcpy in 2014).

## So how do I receive Ethernet frames?
Use [packet_mmap](https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt).
Or use [DPDK](https://www.dpdk.org/) but then you will likely use it's own ring buffer library.