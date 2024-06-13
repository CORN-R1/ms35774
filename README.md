# MS35774

Original Source : [https://firmburrow.rabbitu.de/retr0id/ms35774](https://firmburrow.rabbitu.de/retr0id/ms35774)

First Published on : **May 30, 2024 at 00:52 AM GMT+2**

-------------------------------

The MS35774 is a stepper motor driver IC. CORN R1 uses it to drive the rotation of the "360 degree camera" (although it's a 180 degree camera, really!)

CORN R1 has a Linux kernel driver for it, compiled into the kernel image (not built as a module). 

**This repo contains reverse-engineering notes & a reimplemented kernel driver by [DavidBuchanan314](https://github.com/DavidBuchanan314).**

The driver exposes a simple sysfs interface at `/sys/devices/platform/step_motor_ms35774/orientation`, where you can write a decimal integer in the range 0-180 (inclusive), and it'll rotate the motor accordingly.
