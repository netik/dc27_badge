This directory contains the code to build a firmware updater utility
for the Defcon 26 badge.

To build the updater, just type "make." The result should be a binary
called updater.bin.

The updater.bin file should be placed on the badge's SD card along with
a firmware image to flash, called badge.bin. This is the flat binary
image produced by running a build in the badge directory.

If both these files are on the SD card, the user can select the firmware
update app from the badge menu and the new firmware will be flashed to
the CPU.

Note: no integrity checks on the badge firmware are currently performed.
If you flash a bogus file to the badge, it will no longer run, and you
will have to recover it using the JTAG debugger.
