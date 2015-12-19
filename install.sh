#!/bin/sh

# install DCAN1 firmware

sudo cp BB-DCAN1-00A0.dtbo /lib/firmware
echo BB-DCAN1 | sudo tee /sys/devices/bone_capemgr.*/slots
dmesg | tail -n 14
sudo cat /sys/kernel/debug/pinctrl/44e10800.pinmux/pinmux-pins | grep -i can
