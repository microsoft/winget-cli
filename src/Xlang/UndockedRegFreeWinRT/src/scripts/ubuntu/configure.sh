#!/bin/bash

# run this in script to configure Ubuntu 18.04 with the tools needed to build xlang

# update APT packages
sudo apt update 
sudo apt upgrade -y
sudo apt autoremove -y

# install clang, cmake, ninja and libc++ via APT
sudo apt install clang cmake ninja-build libc++-dev libc++abi-dev -y