# MS35774

The MS35774 is a stepper motor driver IC. The Rabbit R1 uses it to drive the rotation of the "360 degree camera" (although it's a 180 degree camera, really!)

The R1 has a Linux kernel driver for it, compiled into the kernel image (not built as a module). However, Rabbit Inc has not released the source code for this driver, in violation of the Linux kernel's GPL license. (I haven't personally asked for the sources, but I know others have and got no responses, and I'm more interested in reverse engineering than I am in writing emails.)

This repo contains reverse-engineering notes, and maybe one day a reimplemented kernel driver for it.

The driver exposes a simple sysfs interface at `/sys/devices/platform/step_motor_ms35774/orientation`, where you can write a decimal integer in the range 0-180 (inclusive), and it'll rotate the motor accordingly.
