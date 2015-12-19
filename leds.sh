#!/bin/sh

# clear all LED's to turn off the default heartbeat

for led in /sys/class/leds/beaglebone\:green\:usr*; do
  echo none | sudo tee ${led}/trigger
done
