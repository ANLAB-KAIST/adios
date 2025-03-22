# Adios

Source code for "Adios to Busy-Waiting for Microsecond-scale Memory Disaggregation (EuroSys'25)"

Paper: https://yoon.ws/adios

## Prerequisites

* Install Ubuntu 20.04 LTS
* Run all command as `root`. Login into `root` by `sudo su -`

## Install

```bash
sudo su - # login into root
cd /opt
git clone https://github.com/ANLAB-KAIST/adios.git
cd adios

./launch.sh install # install dependencies
./launch.sh install-deps # install ofed

reboot

```

## Configuration

Edit `launch.sh` to meet hardware configuration
(Interface, IP, RDMA, CPU, Node, ETC...)

Must choose right interfaces, RDMA. IP will be set below setup scripts.

## Setup (Always re-run after reboot)

In Compute Node

```bash
./launch.sh setup-compute
```

In Memory Node

```bash
./launch.sh setup-remote
```


In Load generator

```bash
./launch.sh setup-loadgen
```


## Build

```bash
./launch.sh build
```

## Run

Consult functions in `launch.sh`. (do-exp, functions starts with `exp-` and `run-`)

* run-vm-sync (rvs): busy-waiting-based
* run-vm-async-local (rval): yield-based
* run-native (rn): native (w/o unikernel)
