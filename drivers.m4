# Drivers conf part

AC_MSG_CHECKING([which drivers to compile])
AC_ARG_WITH(
  drivers, 
  [  --with-drivers=<list>   compile driver for displays in <list>,]
  [                        drivers may be separated with commas,]	
  [                        'all' (default) compiles all available drivers,]	
  [                        drivers may be excluded with 'all,!<driver>',]	
  [                        (try 'all,\!<driver>' if your shell complains...)]	
  [                        possible drivers are:]	
  [                        BeckmannEgle, CrystalFontz, Curses, Cwlinux,]
  [                        HD44780, M50530, T6963, USBLCD, MatrixOrbital,]
  [                        MilfordInstruments, NULL, PNG, PPM, X11],
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
         BECKMANNEGLE="yes"
         CRYSTALFONTZ="yes"
         CURSES="yes"
         CWLINUX="yes"
         HD44780="yes"
         M50530="yes"
         T6963="yes"
         USBLCD="yes"
         MATRIXORBITAL="yes"
         MILINST="yes"
         NULL="yes" 
         PALMPILOT="yes"
         PNG="yes"
         PPM="yes"
         X11="yes"
         ;;
      BeckmannEgle)
         BECKMANNEGLE=$val
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
      HD44780)
         HD44780=$val
	 ;;
      M50530)
         M50530=$val
         ;;
      NULL)
         NULL=$val;
         ;;
      T6963)
         T6963=$val
         ;;
      USBLCD)
         USBLCD=$val
         ;;
      MatrixOrbital)
         MATRIXORBITAL=$val
         ;;
      MilfordInstruments)
         MILINST=$val
         ;;
      PNG)
         PNG=$val
         ;;
      PPM)
         PPM=$val
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

PARPORT="no"
SERIAL="no"
TEXT="no"
GRAPHIC="no"
IMAGE="no"

if test "$BECKMANNEGLE" = "yes"; then
   DRIVERS="$DRIVERS drv_BeckmannEgle.o"
   AC_DEFINE(WITH_BECKMANNEGLE,1,[Beckmann&Egle driver])
fi

if test "$CRYSTALFONTZ" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_Crystalfontz.o"
   AC_DEFINE(WITH_CRYSTALFONTZ,1,[Crystalfontz driver])
fi

if test "$CURSES" = "yes"; then
   if test "$has_curses" = true; then
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
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_Cwlinux.o"
   AC_DEFINE(WITH_CWLINUX,1,[CwLinux driver])
fi

if test "$HD44780" = "yes"; then
   TEXT="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_HD44780.o"
   AC_DEFINE(WITH_HD44780,1,[HD44780 driver])
fi

if test "$M50530" = "yes"; then
   TEXT="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_M50530.o"
   AC_DEFINE(WITH_M50530,1,[M50530 driver])
fi

if test "$MATRIXORBITAL" = "yes"; then
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_MatrixOrbital.o"
   AC_DEFINE(WITH_MATRIXORBITAL,1,[MatrixOrbital driver])
fi

if test "$MILINST" = "yes"; then
   DRIVERS="$DRIVERS drv_MilfordInstruments.o"
   AC_DEFINE(WITH_MILINST,1,[Milford Instruments driver])
fi

if test "$NULL" = "yes"; then
   DRIVERS="$DRIVERS drv_NULL.o"
   AC_DEFINE(WITH_NULL,1,[NULL driver])
fi

if test "$PNG" = "yes"; then
   if test "$has_gd" = "true"; then
      GRAPHIC="yes"
      IMAGE="yes"
      AC_DEFINE(WITH_PNG,1,[ driver])
      DRVLIBS="$DRVLIBS -lgd"
   else
      AC_MSG_WARN(gd.h not found: PNG driver disabled)
   fi
fi

if test "$PPM" = "yes"; then
   GRAPHIC="yes"
   IMAGE="yes"
   AC_DEFINE(WITH_PPM,1,[ driver])
fi

if test "$IMAGE" = "yes"; then
   DRIVERS="$DRIVERS drv_Image.o"
fi

if test "$T6963" = "yes"; then
   GRAPHIC="yes"
   PARPORT="yes"
   DRIVERS="$DRIVERS drv_T6963.o"
   AC_DEFINE(WITH_T6963,1,[T6963 driver])
fi

if test "$USBLCD" = "yes"; then
   TEXT="yes"
   SERIAL="yes"
   DRIVERS="$DRIVERS drv_USBLCD.o"
   AC_DEFINE(WITH_USBLCD,1,[USBLCD driver])
fi

if test "$X11" = "yes"; then
   if test "$no_x" = "yes"; then
      AC_MSG_WARN(X11 headers or libraries not available: X11 driver disabled)
   else
      GRAPHIC="yes"
      DRIVERS="$DRIVERS drv_X11.o"
      DRVLIBS="$DRVLIBS -L$ac_x_libraries -lX11"
      AC_DEFINE(WITH_X11,1,[X11 driver])
   fi
fi

if test "$DRIVERS" = ""; then
   AC_MSG_ERROR([You should include at least one driver...])
fi
   
# generic text driver
if test "$TEXT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_text.o"
fi

# generic graphic driver
if test "$GRAPHIC" = "yes"; then
:
   DRIVERS="$DRIVERS drv_generic_graphic.o"
fi

# generic parport driver
if test "$PARPORT" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_parport.o"
fi

# generic serial driver
if test "$SERIAL" = "yes"; then
   DRIVERS="$DRIVERS drv_generic_serial.o"
fi

AC_SUBST(DRIVERS)
AC_SUBST(DRVLIBS)
