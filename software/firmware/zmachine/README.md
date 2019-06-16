# A2Z Machine

The A2Z Machine is a Z Machine port to Arduino for playing Zork and other compatible interactive fiction games. It was developed to work with the Adafruit M4 ItsyBitsy Express microcontroller (https://www.adafruit.com/product/3800) with no additional hardware or soldering needed. This project is based on the JZip 2.1 project at http://jzip.sourceforge.net/

For a complete description of the project, visit:

http://danthegeek.com/2018/10/30/a2z-machine-running-zork-on-an-adafruit-itsybitsy-m4-express/

## Quick Start Guide

In addition to code from this GitHub library for the A2Z Machine, you will also need the following libraries installed:

- Adafruit SPIFlash Library - https://github.com/adafruit/Adafruit_SPIFlash
- Adafruit QSPI Library - https://github.com/adafruit/Adafruit_QSPI
- mcurses Library - https://github.com/ChrisMicro/mcurses

You will need to have Circuit Python installed on the ItsyBitsy to manage the filesystem, which gets removed when sketches are uploaded to the device. Circuit Python can be reinstalled by copying a Circuit Python boot image for the ItsyBitsy M4, instructions can be found here:

https://learn.adafruit.com/welcome-to-circuitpython/installing-circuitpython

The filesystem is used to store the game files and saved games. Create 2 folders from the root folder of the device:

- "stories"
- "saves"

Copy the story files from the games folder in the library to the stories folder on the device. 

Create a sketch called "a2z_machine" and copy the project files into the folder. Compile and upload the sketch to the ItsyBitsy.

Use a terminal emulator to play the game (i.e. PuTTY for Windows). BAUD rate should be set to 9600, no local echo.

![ScreenShot](screenshot.png)
