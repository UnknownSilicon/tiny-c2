# tiny-c2
A tiny C2 Framework

# Motivation

The goal of this C2 framework is to create an extremely small, fast framework. This is NOT intended to be extremely modular.
The C2 server is entirely multiprocessed, allowing for fast, non-blocking communications between clients while taking advantage of multi-core CPUs.
This framework is intended to fill a gap in the space, providing an in-between option between basic reverse-shells/proxies and modular, complex C2 frameworks.
The server and clients are designed to be quickly and easily started.


# Security

The initial version of this framework is NOT designed to be secure. There is very basic AES-256 encryption for all packets with hardcoded keys and identifiers.
The communications are not cryptographically secure against determined attackers, especially with access to the client executable.

There is currently no message authentication built in either, potentially allowing replay attacks or other data manipulations.

Edit the AES key in `c2-lib/include/aes.h` to use a non-default key.


# Requirements

- `gcc`
- `cmake`

# Installation

## Option 1 - Build Script

Run `./build.sh`.

Server executable is `build/server/server`, linux client executable is `build/linux-client/linux-client`

## Option 2 - Manual Config and Build

Run `cmake -S . -B build` to configure cmake.
Run `cmake --build build` to build.


# Usage

The server takes two options:

- `-s` or `--server`: Specify the IP to run the server on. Default `0.0.0.0`
- `-p` or `--port`: Specify the port to run the server on. Default `8083`

As of 8/10/2024, the client is designed to connect only to `127.0.0.1:1234` for testing.

After connecting, you can interact with clients through the server's CLI. Run `help` to see commands.
