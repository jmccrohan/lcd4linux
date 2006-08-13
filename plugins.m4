dnl LCD4Linux Plugins conf part
dnl
dnl Copyright (C) 1999, 2000, 2001, 2002, 2003 Michael Reinelt <reinelt@eunet.at>
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

AC_MSG_CHECKING([which plugins to compile])
AC_ARG_WITH(
  plugins, 
  [  --with-plugins=<list>   choose which plugins to compile.]
  [                        type --with-plugins=list for a list]	
  [                        of avaible plugins],
  plugins=$withval, 
  plugins=all
)

plugins=`echo $plugins|sed 's/,/ /g'`

for plugin in $plugins; do

   case $plugin in 
      !*) 
         val="no"
         plugin=`echo $plugin|cut -c 2-`
         ;;
       *) 
         val="yes"
         ;;
   esac
	
   case "$plugin" in
      list)
         AC_MSG_RESULT([TO BE DONE...])
         AC_MSG_ERROR([run ./configure --with-plugins=...])
         ;;
      all)
         PLUGIN_APM="yes"
         PLUGIN_CPUINFO="yes"
         PLUGIN_DISKSTATS="yes"
         PLUGIN_DVB="yes"
         PLUGIN_EXEC="yes"
         PLUGIN_FILE="yes"
         PLUGIN_I2C_SENSORS="yes"
         PLUGIN_IMON="yes"
         PLUGIN_ISDN="yes"
         PLUGIN_KVV="yes"
         PLUGIN_LOADAVG="yes"
         PLUGIN_MEMINFO="yes"
         PLUGIN_MPD="yes"
         PLUGIN_MYSQL="yes"
         PLUGIN_NETDEV="yes"
         PLUGIN_POP3="yes"
         PLUGIN_PPP="yes"
         PLUGIN_PROC_STAT="yes"
         PLUGIN_PYTHON=$with_python
         PLUGIN_SAMPLE="yes"
         PLUGIN_SETI="yes"
         PLUGIN_STATFS="yes"
         PLUGIN_UNAME="yes"
         PLUGIN_UPTIME="yes"
         PLUGIN_WIRELESS="yes"
         PLUGIN_XMMS="yes"   
         ;;
      apm)
         PLUGIN_APM=$val
         ;;
      cpuinfo)
         PLUGIN_CPUINFO=$val
         ;;
      diskstats)
         PLUGIN_DISKSTATS=$val
         ;;
      dvb)
         PLUGIN_DVB=$val
         ;;
      exec)
         PLUGIN_EXEC=$val
         ;;
      file)
         PLUGIN_FILE=$val
         ;;
      i2c_sensors)
         PLUGIN_I2C_SENSORS=$val
	 ;;
      imon)
         PLUGIN_IMON=$val
         ;;
      isdn)
         PLUGIN_ISDN=$val
         ;;
      kvv)
         PLUGIN_KVV=$val
         ;;
      loadavg)
         PLUGIN_LOADAVG=$val
         ;;
      meminfo)
         PLUGIN_MEMINFO=$val
         ;;
      mpd)
         PLUGIN_MPD=$val
	 ;;
      mysql)
         PLUGIN_MYSQL=$val
         ;;
      netdev)
         PLUGIN_NETDEV=$val
         ;;
      pop3)
         PLUGIN_POP3=$val
         ;;
      ppp)
         PLUGIN_PPP=$val
         ;;
      proc_stat)
         PLUGIN_PROC_STAT=$val
         ;;
      python)
         PLUGIN_PYTHON=$val
         ;;
      sample)
         PLUGIN_SAMPLE=$val
         ;;
      seti)
         PLUGIN_SETI=$val
         ;;
      statfs)
         PLUGIN_STATFS=$val
         ;;
      uname)
         PLUGIN_UNAME=$val
         ;;
      uptime)
         PLUGIN_UPTIME=$val
         ;;
      wireless)
         PLUGIN_WIRELESS=$val
         ;;
      xmms)
         PLUGIN_XMMS=$val
         ;;         
      *) 	
         AC_MSG_ERROR([Unknown plugin '$plugin'])
         ;;
   esac
done

AC_MSG_RESULT([done])
if test "$PLUGIN_APM" = "yes"; then
   PLUGINS="$PLUGINS plugin_apm.o"
   AC_DEFINE(PLUGIN_APM,1,[apm plugin])
fi
if test "$PLUGIN_CPUINFO" = "yes"; then
   PLUGINS="$PLUGINS plugin_cpuinfo.o"
   AC_DEFINE(PLUGIN_CPUINFO,1,[cpuinfo plugin])
fi
if test "$PLUGIN_DISKSTATS" = "yes"; then
   PLUGINS="$PLUGINS plugin_diskstats.o"
   AC_DEFINE(PLUGIN_DISKSTATS,1,[diskstats plugin])
fi
if test "$PLUGIN_DVB" = "yes"; then
   AC_CHECK_HEADERS(linux/dvb/frontend.h, [has_dvb_header="true"], [has_dvb_header="false"])
   if test "$has_dvb_header" = "true"; then
      PLUGINS="$PLUGINS plugin_dvb.o"
      AC_DEFINE(PLUGIN_DVB,1,[dvb plugin])
   else
      PLUGINS="$PLUGINS plugin_dvb.o"
      AC_MSG_WARN(linux/dvb/frontend.h header not found: using ioctl)
   fi   
fi
if test "$PLUGIN_EXEC" = "yes"; then
   PLUGINS="$PLUGINS plugin_exec.o"
   AC_DEFINE(PLUGIN_EXEC,1,[exec plugin])
fi
if test "$PLUGIN_FILE" = "yes"; then
   PLUGINS="$PLUGINS plugin_file.o"
   AC_DEFINE(PLUGIN_FILE,1,[file plugin])
fi
if test "$PLUGIN_I2C_SENSORS" = "yes"; then
   PLUGINS="$PLUGINS plugin_i2c_sensors.o"
   AC_DEFINE(PLUGIN_I2C_SENSORS,1,[i2c sensors plugin])
fi
if test "$PLUGIN_IMON" = "yes"; then
   PLUGINS="$PLUGINS plugin_imon.o"
   AC_DEFINE(PLUGIN_IMON,1,[imon plugin])
fi
if test "$PLUGIN_ISDN" = "yes"; then
   AC_CHECK_HEADERS(linux/isdn.h, [has_isdn_header="true"], [has_isdn_header="false"])
   if test "$has_dvb_header" = "false"; then
      AC_MSG_WARN(linux/isdn.h header not found: isdn plugin CPS disabled)
   fi   
   PLUGINS="$PLUGINS plugin_isdn.o"
   AC_DEFINE(PLUGIN_ISDN,1,[ISDN plugin])
fi
if test "$PLUGIN_KVV" = "yes"; then
   PLUGINS="$PLUGINS plugin_kvv.o"
   AC_DEFINE(PLUGIN_KVV,1,[kvv plugin])
fi
if test "$PLUGIN_LOADAVG" = "yes"; then
   PLUGINS="$PLUGINS plugin_loadavg.o"
   AC_DEFINE(PLUGIN_LOADAVG,1,[loadavg plugin])
fi
if test "$PLUGIN_MEMINFO" = "yes"; then
   PLUGINS="$PLUGINS plugin_meminfo.o"
   AC_DEFINE(PLUGIN_MEMINFO,1,[meminfo plugin])
fi
if test "$PLUGIN_MPD" = "yes"; then
   AC_CHECK_HEADERS(libmpd/libmpd.h, [has_libmpd_header="true"], [has_libmpd_header="false"])
   if test "$has_libmpd_header" = "true"; then	
      AC_CHECK_LIB(mpd, mpd_connect, [has_libmpd_lib="true"], [has_libmpd_lib="false"])
      if test "$has_libmpd_lib" = "true"; then
        PLUGINS="$PLUGINS plugin_mpd.o"
        PLUGINLIBS="$PLUGINLIBS -lmpd"
        AC_DEFINE(PLUGIN_MPD,1,[mpd plugin])
      else
        AC_MSG_WARN(libmpd lib not found: mpd plugin disabled)
      fi
   else
      AC_MSG_WARN(libmpd/libmpd.h header not found: mpd plugin disabled)
   fi 
fi
if test "$PLUGIN_MYSQL" = "yes"; then
   AC_CHECK_HEADERS(mysql/mysql.h, [has_mysql_header="true"], [has_mysql_header="false"])
   if test "$has_mysql_header" = "true"; then	
      AC_CHECK_LIB(mysqlclient, mysql_init, [has_mysql_lib="true"], [has_mysql_lib="false"])
      if test "$has_mysql_lib" = "true"; then
        PLUGINS="$PLUGINS plugin_mysql.o"
        PLUGINLIBS="$PLUGINLIBS -lmysqlclient"
        AC_DEFINE(PLUGIN_MYSQL,1,[mysql plugin])
      else
        AC_MSG_WARN(mysqlclient lib not found: mysql plugin disabled)
      fi
   else
      AC_MSG_WARN(mysql/mysql.h header not found: mysql plugin disabled)
   fi 
fi
if test "$PLUGIN_NETDEV" = "yes"; then
   PLUGINS="$PLUGINS plugin_netdev.o"
   AC_DEFINE(PLUGIN_NETDEV,1,[netdev plugin])
fi
if test "$PLUGIN_POP3" = "yes"; then
   PLUGINS="$PLUGINS plugin_pop3.o"
   AC_DEFINE(PLUGIN_POP3,1,[POP3 plugin])
fi
if test "$PLUGIN_PPP" = "yes"; then
   AC_CHECK_HEADERS(net/if_ppp.h, [has_ppp_header="true"], [has_ppp_header="false"])
   if test "$has_ppp_header" = "true"; then
      PLUGINS="$PLUGINS plugin_ppp.o"
      AC_DEFINE(PLUGIN_PPP,1,[ppp plugin])
   else
      AC_MSG_WARN(net/if_ppp.h header not found: ppp plugin disabled)
   fi 
fi
if test "$PLUGIN_PROC_STAT" = "yes"; then
   PLUGINS="$PLUGINS plugin_proc_stat.o"
   AC_DEFINE(PLUGIN_PROC_STAT,1,[proc_stat plugin])
fi
if test "$PLUGIN_PYTHON" = "yes"; then
   if test "$with_python" != "yes"; then
      AC_MSG_WARN(python support not enabled: python plugin disabled (use --with-python to enable))
   else
      if test -z "$python_path"; then
         AC_MSG_WARN(python headers not found: python plugin disabled)
      else
         PLUGINS="$PLUGINS plugin_python.o"
         CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
         PLUGINLIBS="$PLUGINLIBS $PYTHON_LDFLAGS $PYTHON_EXTRA_LIBS"
         AC_DEFINE(PLUGIN_PYTHON,1,[python plugin])
      fi 
   fi 
fi
if test "$PLUGIN_SAMPLE" = "yes"; then
   PLUGINS="$PLUGINS plugin_sample.o"
   AC_DEFINE(PLUGIN_SAMPLE,1,[sample plugin])
fi
if test "$PLUGIN_SETI" = "yes"; then
   PLUGINS="$PLUGINS plugin_seti.o"
   AC_DEFINE(PLUGIN_SETI,1,[seti plugin])
fi
if test "$PLUGIN_STATFS" = "yes"; then
   PLUGINS="$PLUGINS plugin_statfs.o"
   AC_DEFINE(PLUGIN_STATFS,1,[statfs plugin])
fi
if test "$PLUGIN_UNAME" = "yes"; then
   PLUGINS="$PLUGINS plugin_uname.o"
   AC_DEFINE(PLUGIN_UNAME,1,[uname plugin])
fi
if test "$PLUGIN_UPTIME" = "yes"; then
   PLUGINS="$PLUGINS plugin_uptime.o"
   AC_DEFINE(PLUGIN_UPTIME,1,[uptime plugin])
fi
if test "$PLUGIN_WIRELESS" = "yes"; then
   PLUGINS="$PLUGINS plugin_wireless.o"
   AC_DEFINE(PLUGIN_WIRELESS,1,[wireless plugin])
fi
if test "$PLUGIN_XMMS" = "yes"; then
   PLUGINS="$PLUGINS plugin_xmms.o"
   AC_DEFINE(PLUGIN_XMMS,1,[xmms plugin])
fi

#if test "$PLUGIN_" = "yes"; then
#   PLUGINS="$PLUGINS plugin_.o"
#   AC_DEFINE(PLUGIN_,1,[plugin])
#fi

if test "$PLUGINS" = ""; then
   AC_MSG_ERROR([You should include at least one plugin...])
#else
#   AC_MSG_ERROR($PLUGINS)
fi
   
AC_SUBST(PLUGINS)
AC_SUBST(PLUGINLIBS)
