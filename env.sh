#!/bin/bash

# check the necessary tools
if ! command -v bison > /dev/null 2>&1; then
    echo "bison is not installed. Installing..."
    sudo apt update
    sudo apt install bison
else
    echo "bison is installed."
fi

if ! command -v flex > /dev/null 2>&1; then
    echo "flex is not installed. Installing..."
    sudo apt update
    sudo apt install flex
else
    echo "flex is installed."
fi

if dpkg -l | grep -q libncurses-dev; then
    echo "libncurses-dev is installed."
else
    echo "libncurses-dev is not installed. Installing..."
    sudo apt update
    sudo apt install -y libncurses-dev
fi

if dpkg -l | grep -q libreadline-dev; then
    echo "libreadline-dev is installed."
else
    echo "libreadline-dev is not installed. Installing..."
    sudo apt update
    sudo apt install -y libreadline-dev
fi

# set enviroment
export NEMU_HOME=$(pwd)
echo "set NEMU_HOME=${NEMU_HOME}"
export AM_HOME=$(pwd)/../abstract-machine
echo "set AM_HOME=${AM_HOME}"
