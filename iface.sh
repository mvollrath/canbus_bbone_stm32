#!/bin/sh

# set up real and virtual CAN interfaces

BITRATE=${1:-500000}

sudo ip link add can0 type can
sudo ip link set can0 up type can loopback off bitrate ${BITRATE}
sudo ifconfig can0 up

ip link show up dev can0
ifconfig can0

sudo modprobe vcan
sudo ip link add vcan0 type vcan
sudo ifconfig vcan0 up

ip link show up dev vcan0
ifconfig vcan0
