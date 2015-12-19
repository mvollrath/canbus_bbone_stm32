### Video

I swear this works at least a little bit, here look:

<https://www.youtube.com/watch?v=pqwqNU--IbQ>

### Enabling CAN Bus on BeagleBone Black

You'll need to compile and install a dtbo.

<http://www.embedded-things.com/bbb/enable-canbus-on-the-beaglebone-black/>

There are a couple of scripts that can do this for you:

```
./build.sh
./install.sh
```

### DIY CAN Bus Cape

This describes how to put together a CAN Bus transceiver project board.

<http://www.instructables.com/id/DIY-Beaglebone-CAN-Bus-Cape/>

### SocketCAN

The canping and canpong binaries use the excellent [SocketCAN](https://www.kernel.org/doc/Documentation/networking/can.txt) interface.  Once DCAN1 is enabled, use `iface.sh` to set up `can0` and `vcan0` interfaces.

### Preparing LED's on BeagleBone Black

Use `leds.sh` to disable the default LED triggers on the BBB.

### Building canping and canpong

Shouldn't need anything special.

```
gcc -o canping canping.c
gcc -o canpong canpong.c
```

### Testing canping and canpong

As long as `vcan0` is available, this should give you some blinkage.

```
canping -n vcan0 &
sudo canpong -n vcan0 &
```

Kill everything with `pkill canping` and `sudo pkill canpong` when you're done.

### Building and burning the STM32F4 binary

I used [stlink](https://github.com/texane/stlink) to build and burn to the STM32F4 board from the BBB.  Follow the README in that repo to set up the toolchain.

Set the STLINK path in `stm32_canpong/Makefile` and try a build:

```
cd stm32_canpong
make
```

If that works, burn it:

```
make burn
```

Reset the board and, if everything is hooked up correctly and you're lucky, you should see lights blinking in concert.  Be sure to startup `canping` and `canpong` against `can0`.

### Debugging

The `candump` tool is really handy for spitting out all CAN messages that the BBB can see:

```
candump can0
```

