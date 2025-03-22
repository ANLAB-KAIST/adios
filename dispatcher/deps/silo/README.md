## Overview

This repository contains a simple TCP server written using the Shenango runtime that executes TPC-C transactions in an in-memory Silo database.

## Build
Run `./silo.sh` to intialize and build submodules.

Run `SHENANGODIR=<path_to_shenango> make` to compile the server.

## Run

To launch the server, run:
`./silotpcc-shenango [cfg_file] [threads] [port] [memory]`