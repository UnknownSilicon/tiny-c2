#!/bin/bash

export CC=clang
export CXX=clang++

cmake -S . -B build

cmake --build build