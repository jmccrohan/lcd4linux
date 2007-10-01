#! /bin/bash

#  $Id$
#  $URL$


rm -f smoketest.log lcd4linux

make distclean
./bootstrap

for driver in BeckmannEgle BWCT CrystalFontz Curses Cwlinux EA232graphic G15 HD44780 LCD2USB LCDLinux LCDTerm LEDMatrix LPH7508 LUIse M50530 MatrixOrbital MilfordInstruments Noritake NULL Pertelian picoLCD PNG PPM RouterBoard Sample serdisplib SimpleLCD T6963 Trefon USBHUB USBLCD WincorNixdorf X11; do

    make distclean
    ./configure --with-drivers=$driver
    make
    
    if [ -x lcd4linux ]; then
	echo "Success: $driver" >>smoketest.log
    else
	echo "FAILED:  $driver" >>smoketest.log
    fi
    
done

make distclean
./configure
