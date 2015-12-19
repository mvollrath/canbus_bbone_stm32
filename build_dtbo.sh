#!/bin/sh

# build DCAN1 firmware

dtc -O dtb -o BB-DCAN1-00A0.dtbo -b 0 -@ BB-DCAN1-00A0.dts
