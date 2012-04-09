dnl $Id$
dnl $URL$


dnl LCD4Linux Drivers conf part
dnl
dnl Copyright (C) 1999, 2000, 2001, 2002, 2003 Michael Reinelt <michael@reinelt.co.at>
dnl Copyright (C) 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
dnl
dnl This file is part of LCD4Linux.
dnl
dnl LCD4Linux is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl LCD4Linux is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

AC_MSG_CHECKING([which drivers to compile])
AC_ARG_WITH(
  drivers,
  [  --with-drivers=<list>   compile driver for displays in <list>,]
  [                        drivers may be separated with commas,]
  [                        'all' (default) compiles all available drivers,]
  [                        drivers may be excluded with 'all,!<driver>',]
  [                        (try 'all,\!<driver>' if your shell complains...)]
  [                        possible drivers are:]
  [                        ASTUSB, BeckmannEgle, BWCT, CrystalFontz, Curses, Cwlinux, D4D, DPF]
  [                        EA232graphic, EFN, FutabaVFD, FW8888, G15, GLCD2USB, HD44780, HD44780-I2C,]
  [                        IRLCD, LCD2USB, LCDLinux, LEDMatrix, LCDTerm, LPH7508, LUIse,]
  [                        LW_ABP, M50530, MatrixOrbital, MatrixOrbitalGX, MilfordInstruments, MDM166A,]
  [                        Newhaven, Noritake, NULL, Pertelian, PHAnderson,]
  [                        PICGraphic, picoLCD, picoLCDGraphic, PNG, PPM, RouterBoard,]
  [                        Sample, SamsungSPF, serdisplib, ShuttleVFD, SimpleLCD, st2205, T6963,]
  [                        TeakLCM, Trefon, ULA200, USBHUB, USBLCD, VNC, WincorNixdorf, X11],
  drivers=$withval,
  drivers=all
)

drivers=`echo $drivers|sed 's/,/ /g'`

for driver in $drivers; do

   case $driver in
      !*)
         val="no"
         driver=`echo $driver|cut -c 2-`
         ;;
       *)
         val="yes"
         ;;
   esac

   case "$driver" in
      all)
         ASTUSB="yes"
         BECKMANNEGLE="yes"
         BWCT="yes"
         CRYSTALFONTZ="yes"
         CURSES="yes"
         CWLINUX="yes"
         D4D="yes"
         DPF="yes"
         EA232graphic="yes"
         EFN="yes"
         FUTABAVFD="yes"
         FW8888="yes"
         G15="yes"
         GLCD2USB="yes"
         HD44780="yes"
         HD44780_I2C="yes"
	 IRLCD="yes"
         LCD2USB="yes"
	 LCDLINUX="yes"
         LCDTERM="yes"
         LEDMATRIX="yes"
	 LPH7508="yes"
         LUISE="yes"
         LW_ABP="yes"
         M50530="yes"
         MATRIXORBITAL="yes"
         MATRIXORBITALGX="yes"
         MDM166A="yes"
         MILINST="yes"
         NEWHAVEN="yes"
         NORITAKE="yes"
         NULL="yes"
         PERTELIAN="yes"
         PHANDERSON="yes"
         PICGRAPHIC="yes"
         PICOLCD="yes"
	 PICOLCDGRAPHIC="yes"
         PNG="yes"
         PPM="yes"
         ROUTERBOARD="yes"
         SAMPLE="yes"
         SAMSUNGSPF="yes"
         ST2205="yes"
	 SERDISPLIB="yes"
	 SHUTTLEVFD="yes"
         SIMPLELCD="yes"
         T6963="yes"
         TeakLCM="yes"
         Trefon="yes"
         ULA200="yes"
	 USBHUB="yes"
         USBLCD="yes"
         VNC="yes"
	 WINCORNIXDORF="yes"
         X11="yes"
         ;;
      ASTUSB)
         ASTUSB=$val
         ;;
      BeckmannEgle)
         BECKMANNEGLE=$val
         ;;
      BWCT)
         BWCT=$val
         ;;
      CrystalFontz)
         CRYSTALFONTZ=$val
         ;;
      Curses)
         CURSES=$val
         ;;
      Cwlinux)
         CWLINUX=$val
         ;;
      D4D)
         D4D=$val
         ;;
      DPF)
         DPF=$val
         ;;
      EA232graphic)
         EA232graphic=$val
         ;;
      EFN)
         EFN=$val
	 ;;
      FutabaVFD)
         FUTABAVFD=$val
         ;;
      FW8888)
         FW8888=$val
         ;;
      G15)
         G15=$val
         ;;
      GLCD2USB)
         GLCD2USB=$val
         ;;
      HD44780)
         HD44780=$val
	 ;;
      HD44780-I2C)
         HD44780_I2C=$val
	 ;;
      IRLCD)
         IRLCD=$val
         ;;
      LCD2USB)
         LCD2USB=$val
         ;;
      LCDLinux)
         LCDLINUX=$val
	 ;;
      LCDTerm)
         LCDTERM=$val
	 ;;
      LEDMatrix)
         LEDMATRIX=$val
	 ;;
      LPH7508)
         LPH7508=$val
         ;;
      LUIse)
         LUISE=$val
         ;;
      LW_ABP)
         LW_ABP=$val
         ;;
      M50530)
         M50530=$val
         ;;
      MatrixOrbital)
         MATRIXORBITAL=$val
         ;;
      MatrixOrbitalGX)
         MATRIXORBITALGX=$val
         ;;
      MDM166A)
         MDM166A=$val
         ;;
      MilfordInstruments)
         MILINST=$val
         ;;
      Newhaven)
         NEWHAVEN=$val
         ;;
      Noritake)
         NORITAKE=$val;
         ;;
      NULL)
         NULL=$val;
         ;;
      Pertelian)
         PERTELIAN=$val
         ;;
      PHAnderson)
         PHANDERSON=$val
         ;;
      PICGraphic)
         PICGRAPHIC=$val
         ;;
      picoLCD)
         PICOLCD=$val
         ;;
      picoLCDGraphic)
         PICOLCDGRAPHIC=$val
         ;;
      PNG)
         PNG=$val
         ;;
      PPM)
         PPM=$val
         ;;
      RouterBoard)
         ROUTERBOARD=$val
         ;;
      Sample)
         SAMPLE=$val
         ;;
      SamsungSPF)
         SAMSUNGSPF=$val
         ;;
      serdisplib)
         SERDISPLIB=$val;
         ;;
      ShuttleVFD) 
	 SHUTTLEVFD=$val
	 ;;          
      SimpleLCD)
         SIMPLELCD=$val
         ;;
      st2205)
         ST2205=$val
         ;;
      T6963)
         T6963=$val
         ;;
      TeakLCM)
         TeakLCM=$val
         ;;
      Trefon)
         Trefon=$val
         ;;
      ULA200)
         ULA200=$val
         ;;
      USBHUB)
         USBHUB=$val
         ;;
      USBLCD)
         USBLCD=$val
         ;;
      VNC)
         VNC=$val
         ;;
      WincorNixdorf)
         WINCORNIXDORF=$val
         ;;
      X11)
         X11=$val
         ;;
      *)
         AC_MSG_ERROR([Unknown driver '$driver'])
         ;;
   esac
done

AC_MSG_RESULT([done])


# generic display drivers
TEXT="no"
GRAPHIC="no"
IMAGE="no"
GPIO="no"

# generiv I/O drivers
PARPORT="no"
SERIAL="no"
I2C="no"
KEYPAD="no"

# generic libraries
LIBUSB="no"
LIBUSB10="no"
LIBFTDI="no"

if test "$ASTUSB" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      SERIAL="yes"
      DRIVERS="$DRIVERS drv_ASTUSB.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_ASTUSB,1,[ASTUSB driver])
   else
      AC_MSG_WARN(usb.h not found: ASTUSB driver disabled)
   fi
fi

if test "$BECKMANNEGLE" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_BeckmannEgle.o"
   AC_DEFINE(WITH_BECKMANNEGLE,1,[Beckmann&Egle driver])
fi

if test "$BWCT" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_BWCT.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_BWCT,1,[BWCT driver])
   else
      AC_MSG_WARN(usb.h not found: BWCT driver disabled)
   fi
fi

if test "$CRYSTALFONTZ" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   KEYPAD="yes"
   DRIVERS="$DRIVERS drv_Crystalfontz.o"
   AC_DEFINE(WITH_CRYSTALFONTZ,1,[Crystalfontz driver])
fi

if test "$CURSES" = "yes"; then
   if test "$has_curses" = true; then
      TEXT="yes"
      KEYPAD="yes"
      DRIVERS="$DRIVERS drv_Curses.o"
      DRVLIBS="$DRVLIBS $CURSES_LIBS"
      CPPFLAGS="$CPPFLAGS $CURSES_INCLUDES"
      AC_DEFINE(WITH_CURSES,1,[Curses driver])
   else
      AC_MSG_WARN(curses not found: Curses driver disabled)
   fi
fi

if test "$CWLINUX" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   KEYPAD="yes"
   DRIVERS="$DRIVERS drv_Cwlinux.o"
   AC_DEFINE(WITH_CWLINUX,1,[CwLinux driver])
fi

if test "$D4D" = "yes"; then
   TEXT="yes"
   GRAPHIC="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_D4D.o"
   AC_DEFINE(WITH_D4D,1,[D4D driver])
fi

if test "$DPF" = "yes"; then
   if test "$has_libdpf" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_dpf.o"
      DRVLIBS="$DRVLIBS -Llibdpf -ldpf -lusb"
      AC_DEFINE(WITH_DPF,1,[DPF driver])
   else
      AC_MSG_WARN(libdpf.h not found: DPF driver disabled)
   fi
fi

if test "$EA232graphic" = "yes"; then
   GRAPHIC="yes"
   SERIAL="yes"
   GPIO="yes"
   DRIVERS="$DRIVERS drv_EA232graphic.o"
   AC_DEFINE(WITH_EA232graphic,1,[Electronic Assembly RS232 graphic driver])
fi

if test "$EFN" = "yes"; then
   TEXT="yes"
   DRIVERS="$DRIVERS drv_EFN.o"
   AC_DEFINE(WITH_EFN,1,[Driver for EFN LED modules and EUG 100 ethernet to serial converter])
fi

if test "$FUTABAVFD" = "yes"; then
   if test "$has_parport" = "true"; then
      TEXT="yes"
      # select bus: serial (including USB), parallel or i2c
      PARPORT="yes"
      DRIVERS="$DRIVERS drv_FutabaVFD.o"
      AC_DEFINE(WITH_FUTABAVFD,1,[FutabaVFD driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: FutabaVFD driver disabled)
   fi
fi



if test "$FW8888" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_FW8888.o"
   AC_DEFINE(WITH_FW8888,1,[Allnet FW8888 driver])
fi

if test "$G15" = "yes"; then
   if test "$has_usb" = "true"; then
      GRAPHIC="yes"
      LIBUSB="yes"
      DRIVERS="$DRIVERS drv_G15.o"
      AC_DEFINE(WITH_G15,1,[G-15 driver])
   else
      AC_MSG_WARN(usb.h not found: G15 driver disabled)
   fi
fi

if test "$GLCD2USB" = "yes"; then
   if test "$has_usb" = "true"; then
      GRAPHIC="yes"
      KEYPAD="yes"
      DRIVERS="$DRIVERS drv_GLCD2USB.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_GLCD2USB,1,[GLCD2USB driver])
   else
      AC_MSG_WARN(usb.h not found: GLCD2USB driver disabled)
   fi
fi

if test "$HD44780_I2C" = "yes"; then
   TEXT="yes"
   I2C="yes"
   GPIO="yes"
   DRIVERS="$DRIVERS drv_HD44780.o"
   AC_DEFINE(WITH_HD44780,1,[HD44780 driver])
fi

if test "$HD44780" = "yes"; then
   if test "$HD44780_I2C" != "yes"; then
      if test "$has_parport" = "true"; then
         TEXT="yes"
         PARPORT="yes"
         I2C="yes"
         GPIO="yes"
         KEYPAD="yes"
         DRIVERS="$DRIVERS drv_HD44780.o"
         AC_DEFINE(WITH_HD44780,1,[HD44780 driver])
      else
         AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: HD44780 driver disabled)
      fi
   else
      HD44780="no"
      AC_MSG_WARN(HD44780-i2c enabled disabling HD44780)
   fi
fi

if test "$IRLCD" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      SERIAL="yes"
      DRIVERS="$DRIVERS drv_IRLCD.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_IRLCD,1,[IRLCD driver])
   else
      AC_MSG_WARN(usb.h not found: IRLCD driver disabled)
   fi
fi

if test "$LCD2USB" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      SERIAL="yes"
      KEYPAD="yes"
      DRIVERS="$DRIVERS drv_LCD2USB.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_LCD2USB,1,[LCD2USB driver])
   else
      AC_MSG_WARN(usb.h not found: LCD2USB driver disabled)
   fi
fi

if test "$LCDLINUX" = "yes"; then
   if test "$has_lcd_linux" = true; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_LCDLinux.o"
      AC_DEFINE(WITH_LCDLINUX,1,[LCD-Linux driver])
   else
      AC_MSG_WARN(linux/lcd-linux.h or linux/hd44780.h not found: LCD-Linux driver disabled)
   fi
fi

if test "$LCDTERM" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_LCDTerm.o"
   AC_DEFINE(WITH_LCDTERM,1,[LCDTerm driver])
fi

if test "$LEDMATRIX" = "yes"; then
   GRAPHIC="yes"
   DRIVERS="$DRIVERS drv_LEDMatrix.o"
   AC_DEFINE(WITH_LEDMATRIX,1,[LEDMatrix driver])
fi

if test "$LPH7508" = "yes"; then
   if test "$has_parport" = "true"; then
      GRAPHIC="yes"
      GPIO="yes"
      PARPORT="yes"
      DRIVERS="$DRIVERS drv_LPH7508.o"
      AC_DEFINE(WITH_LPH7508,1,[LPH7508 driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: LPH7508 driver disabled)
   fi
fi

if test "$LUISE" = "yes"; then
   if test "$has_luise" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_LUIse.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lluise"
      AC_DEFINE(WITH_LUISE,1,[LUIse driver])
   else
      AC_MSG_WARN(luise.h not found: LUIse driver disabled)
   fi
fi

if test "$LW_ABP" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   KEYPAD="yes"
   DRIVERS="$DRIVERS drv_LW_ABP.o"
   AC_DEFINE(WITH_LW_ABP,1,[LW ABP driver])
fi

if test "$M50530" = "yes"; then
   if test "$has_parport" = "true"; then
      TEXT="yes"
      GPIO="yes"
      PARPORT="yes"
      DRIVERS="$DRIVERS drv_M50530.o"
      AC_DEFINE(WITH_M50530,1,[M50530 driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: M50530 driver disabled)
   fi
fi

if test "$MATRIXORBITAL" = "yes"; then
   TEXT="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_MatrixOrbital.o"
   AC_DEFINE(WITH_MATRIXORBITAL,1,[MatrixOrbital driver])
fi

if test "$MATRIXORBITALGX" = "yes"; then
    if test "$has_usb" = "true"; then
        GRAPHIC="yes"
        SERIAL="yes"
        LIBUSB="yes"
        DRIVERS="$DRIVERS drv_MatrixOrbitalGX.o"
        AC_DEFINE(WITH_MATRIXORBITALGX,1,[MatrixOrbitalGX driver])
    else
        AC_MSG_WARN(usb.h not found: MatrixOrbitalGX driver disabled)
    fi
fi

if test "$MDM166A" = "yes"; then 
   if test "$has_usb10" = "true"; then 
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_mdm166a.o" 
      GPIO="yes" 
      LIBUSB10="yes"
      AC_DEFINE(WITH_MDM166A,1,[MDM166A driver]) 
    else 
      AC_MSG_WARN(libusb-1.0/libusb.h not found: MDM166A driver disabled) 
    fi 
fi 

if test "$MILINST" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_MilfordInstruments.o"
   AC_DEFINE(WITH_MILINST,1,[Milford Instruments driver])
fi

if test "$NEWHAVEN" = "yes"; then
   TEXT="yes"
   I2C="yes"
   DRIVERS="$DRIVERS drv_Newhaven.o"
   AC_DEFINE(WITH_NEWHAVEN,1,[Newhaven driver])
fi

if test "$NORITAKE" = "yes"; then
   if test "$has_parport" = "true"; then
      TEXT="yes"
      GRAPHIC="yes"
      PARPORT="yes"
      DRIVERS="$DRIVERS drv_Noritake.o"
      AC_DEFINE(WITH_NORITAKE,1,[Noritake driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: NORITAKE driver disabled)
   fi
fi

if test "$NULL" = "yes"; then
   TEXT="yes"
   DRIVERS="$DRIVERS drv_NULL.o"
   AC_DEFINE(WITH_NULL,1,[NULL driver])
fi

if test "$PERTELIAN" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_Pertelian.o"
   AC_DEFINE(WITH_PERTELIAN,1,[Pertelian driver])
fi

if test "$PHANDERSON" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_PHAnderson.o"
   AC_DEFINE(WITH_PHANDERSON,1,[PHAnderson driver])
fi

if test "$PICGRAPHIC" = "yes"; then
   GRAPHIC="yes"
   GPIO="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_PICGraphic.o"
   AC_DEFINE(WITH_PICGRAPHIC,1,[PICGraphic driver])
fi

if test "$PICOLCD" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      GPIO="yes"
      SERIAL="yes"
      LIBUSB="yes"
      DRIVERS="$DRIVERS drv_picoLCD.o"
      AC_DEFINE(WITH_PICOLCD,1,[picoLCD driver])
   else
      AC_MSG_WARN(usb.h not found: picoLCD driver disabled)
   fi
fi

if test "$PICOLCDGRAPHIC" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      GRAPHIC="yes"
      KEYPAD="yes"      
      GPIO="yes"
      SERIAL="yes"
      LIBUSB="yes"
      DRIVERS="$DRIVERS drv_picoLCDGraphic.o"
      AC_DEFINE(WITH_PICOLCDGRAPHIC,1,[picoLCDGraphic driver])
   else
      AC_MSG_WARN(usb.h not found: picoLCDGraphic driver disabled)
   fi
fi

if test "$PNG" = "yes"; then
   if test "$has_gd" = "true"; then
      IMAGE="yes"
      AC_DEFINE(WITH_PNG,1,[PNG driver])
   else
      AC_MSG_WARN(gd.h not found: PNG driver disabled)
   fi
fi

if test "$PPM" = "yes"; then
   IMAGE="yes"
   AC_DEFINE(WITH_PPM,1,[PPM driver])
fi

if test "$ROUTERBOARD" = "yes"; then
   if test "$has_io_h" = "true"; then
      TEXT="yes"
      GPIO="yes"
      DRIVERS="$DRIVERS drv_RouterBoard.o"
      AC_DEFINE(WITH_ROUTERBOARD,1,[RouterBoard driver])
   else
      AC_MSG_WARN(sys/io.h not found: RouterBoard driver disabled)
   fi
fi

if test "$SAMPLE" = "yes"; then
   if test "$has_parport" = "true"; then
      # select either text or graphics mode
      TEXT="yes"
      GRAPHIC="yes"
      # support for GPIO's
      GPIO="yes"
      # select bus: serial (including USB), parallel or i2c
      SERIAL="yes"
      PARPORT="yes"
      #I2C="yes"
      DRIVERS="$DRIVERS drv_Sample.o"
      AC_DEFINE(WITH_SAMPLE,1,[Sample driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: SAMPLE driver disabled)
   fi
fi

if test "$SAMSUNGSPF" = "yes"; then
   if test "$has_usb" = "true"; then 
      if test "$has_jpeglib" = "true"; then 
      	 GRAPHIC="yes"
      	 DRIVERS="$DRIVERS drv_SamsungSPF.o"
      	 LIBUSB="yes" 
	 LIBJPEG="yes"
      	 AC_DEFINE(WITH_SAMSUNGSPF,1,[SamsungSPF driver])
      else
        AC_MSG_WARN(jpeglib.h not found: SamsungSPF driver disabled) 
      fi
   else
      AC_MSG_WARN(usb.h not found: SamsungSPF driver disabled) 
   fi
fi

if test "$SERDISPLIB" = "yes"; then
   if test "$has_serdisplib" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_serdisplib.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lserdisp"
      AC_DEFINE(WITH_SERDISPLIB,1,[serdisplib driver])
      if test "$has_usb" = "true"; then
         LIBUSB="yes"
      fi
   else
      AC_MSG_WARN(serdisp.h not found: serdisplib driver disabled)
   fi
fi

if test "$SHUTTLEVFD" = "yes"; then 
   if test "$has_usb" = "true"; then 
      TEXT="yes" 
      GPIO="yes" 
      DRIVERS="$DRIVERS drv_ShuttleVFD.o" 
      LIBUSB="yes" 
      AC_DEFINE(WITH_SHUTTLEVFD,1,[ShuttleVFD driver]) 
    else 
      AC_MSG_WARN(usb.h not found: ShuttleVFD driver disabled) 
    fi 
fi 

if test "$SIMPLELCD" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_SimpleLCD.o"
   AC_DEFINE(WITH_SIMPLELCD,1,[SimpleLCD driver])
fi

if test "$ST2205" = "yes"; then
   if test "$has_st2205" = "true"; then
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_st2205.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lst2205"
      AC_DEFINE(WITH_ST2205,1,[st2205 driver])
   else
      AC_MSG_WARN(st2205.h not found: st2205 driver disabled)
   fi
fi


if test "$T6963" = "yes"; then
   if test "$has_parport" = "true"; then
      GRAPHIC="yes"
      PARPORT="yes"
      DRIVERS="$DRIVERS drv_T6963.o"
      AC_DEFINE(WITH_T6963,1,[T6963 driver])
   else
      AC_MSG_WARN(asm/io.h or {linux/parport.h and linux/ppdev.h} not found: T6963 driver disabled)
   fi
fi

if test "$TeakLCM" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_TeakLCM.o"
   AC_DEFINE(WITH_TEAK_LCM,1,[TeakLCM driver])
fi

if test "$Trefon" = "yes"; then
   if test "$has_usb" = "true"; then
      TEXT="yes"
      DRIVERS="$DRIVERS drv_Trefon.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_TREFON,1,[TREFON driver])
   else
      AC_MSG_WARN(usb.h not found: Trefon driver disabled)
   fi
fi

if test "$ULA200" = "yes"; then
   if test "$has_ftdi" = "true"; then
      TEXT="yes"
      LIBUSB="yes"
      LIBFTDI="yes"
      DRIVERS="$DRIVERS drv_ula200.o"
      AC_DEFINE(WITH_ULA200,1,[ULA200 driver])
   else
      AC_MSG_WARN(ftdi.h not found: ULA200 driver disabled)
   fi
fi

if test "$USBHUB" = "yes"; then
   if test "$has_usb" = "true"; then
      GPIO="yes"
      DRIVERS="$DRIVERS drv_USBHUB.o"
      LIBUSB="yes"
      AC_DEFINE(WITH_USBHUB,1,[USBHUB driver])
   else
      AC_MSG_WARN(usb.h not found: USB-Hub driver disabled)
   fi
fi

if test "$USBLCD" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_USBLCD.o"
   if test "$has_usb" = "true"; then
      LIBUSB="yes"
   fi
   AC_DEFINE(WITH_USBLCD,1,[USBLCD driver])
fi

if test "$VNC" = "yes"; then
   if test "$has_vncserverlib" = "true"; then
      GRAPHIC="yes"
      KEYPAD="yes"      
      DRIVERS="$DRIVERS drv_vnc.o"
      DRVLIBS="$DRVLIBS -L/usr/local/lib -lvncserver -lz"
      AC_DEFINE(WITH_VNC,1,[vnc driver])
   else
      AC_MSG_WARN(libvncserver not found: vnc driver disabled)
   fi
fi

if test "$WINCORNIXDORF" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_WincorNixdorf.o"
   AC_DEFINE(WITH_WINCORNIXDORF,1,[WincorNixdorf driver])
fi

if test "$X11" = "yes"; then
   if test "$no_x" = "yes"; then
      AC_MSG_WARN(X11 headers or libraries not available: X11 driver disabled)
   else
      GRAPHIC="yes"
      KEYPAD="yes"
      DRIVERS="$DRIVERS drv_X11.o"
      if test "x$ac_x_libraries" = "x"; then
	DRVLIBS="$DRVLIBS -lX11"
      else
        DRVLIBS="$DRVLIBS -L$ac_x_libraries -lX11"
      fi
      CPP_FLAGS="$CPPFLAGS $X_CFLAGS"
      AC_DEFINE(WITH_X11, 1, [X11 driver])
   fi
fi


# Image driver
if test "$IMAGE" = "yes"; then
   GRAPHIC="yes"
   DRIVERS="$DRIVERS drv_Image.o"
fi

if test "$DRIVERS" = ""; then
   AC_MSG_ERROR([You should activate at least one driver...])
fi

# generic text driver
if test "$TEXT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_text.o"
fi

# generic graphic driver
if test "$GRAPHIC" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_graphic.o"
   if test "$has_gd" = "true"; then
      DRIVERS="$DRIVERS widget_image.o"
      DRVLIBS="$DRVLIBS -lgd"
      AC_DEFINE(WITH_GD, 1, [GD library])
      AC_DEFINE(WITH_IMAGE, 1, [image widget])
   fi
fi

# generic GPIO driver
if test "$GPIO" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_gpio.o"
fi

# generic parport driver
if test "$PARPORT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_parport.o"
   AC_DEFINE(WITH_PARPORT, 1, [parport bus driver])
fi

# generic serial driver
if test "$SERIAL" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_serial.o"
   AC_DEFINE(WITH_SERIAL, 1, [serial bus driver])
fi

# generic i2c driver
if test "$I2C" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_i2c.o"
   AC_DEFINE(WITH_I2C, 1, [I2C bus driver])
fi

# generic keypad driver
if test "$KEYPAD" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_keypad.o"
fi

# libjpeg
if test "$LIBJPEG" = "yes"; then
   DRVLIBS="$DRVLIBS -ljpeg"
fi

# libusb
if test "$LIBUSB" = "yes"; then
   DRVLIBS="$DRVLIBS -lusb"
fi

# libusb-1.0
if test "$LIBUSB10" = "yes"; then
   DRVLIBS="$DRVLIBS -lusb-1.0"
fi

# libftdi
if test "$LIBFTDI" = "yes"; then
   DRVLIBS="$DRVLIBS -lftdi"
fi

if test "$DRIVERS" = ""; then
   AC_MSG_ERROR([You should include at least one driver...])
fi
   
AC_SUBST(DRIVERS)
AC_SUBST(DRVLIBS)
