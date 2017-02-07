# NTUA OSLab
Operating Systems Laboratory Assignments

## Overview
Assignments for Operating Systems Laboratory class at National Technical University of Athens (NTUA).

Linux kernel driver development, QEMU development, virtualization techniques.

### Lab1
Implementation of a Linux kernel driver for a wireless sensor network. This network consists of  a number sensors residing inside a lab room measuring battery voltage, temperature and light levels. Developed a character device driver which receives in raw format the data from the sensor network, filters and exposes the information to userspace in different device files for each metric. The driver was developed as a Linux kernel module in virtual machines on top of [okeanos](https://okeanos.grnet.gr/home/)' infrastructure.

### Lab2
Developed virtual hardware for QEMU-KVM framework. Designed and implemented a virtual VirtIO cryptographic device as part of QEMU. The device allows applications running inside the VM to access the real host crypto device using paravirtualization. The device was implemented using the split-driver model: a frontend (inside the VM) and a backend (part of QEMU). The frontend exposes the same API to userspace applications as the host cryptodev while the backend receives calls from the frontend and forwards them for processing by the host cryptodev. In order to test the driver's functionality, an encrypted chat application over TCP/IP sockets was implemented allowing the communication between 2 Virtual Machines that were residing in the same Host Machine.

### Lab3
A riddle game containing several challenges testing the proper use of system calls, Process-Handling and Inter-Process Communication (IPC), Process Scheduling, Network Programming and Systems Programming fundamentals.

## Requirements
* CPU supporting virtualization extensions
* KVM module loaded into kernel
* QEMU version 1.2.1 or newer
