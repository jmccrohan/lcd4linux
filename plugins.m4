# Plugins conf part

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
         driver=`echo $plugin|cut -c 2-`
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
         PLUGIN_I2C_SENSORS="yes"
         PLUGIN_IMON="yes"
         PLUGIN_ISDN="yes"
         PLUGIN_LOADAVG="yes"
         PLUGIN_MEMINFO="yes"
         PLUGIN_MYSQL="yes"
         PLUGIN_NETDEV="yes"
         PLUGIN_POP3="yes"
         PLUGIN_PPP="yes"
         PLUGIN_PROC_STAT="yes"
         PLUGIN_SETI="yes"
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
      i2c_sensors)
         PLUGIN_I2C_SENSORS=$val
	 ;;
      imon)
         PLUGIN_IMON=$val
         ;;
      isdn)
         PLUGIN_ISDN=$val
         ;;
      loadavg)
         PLUGIN_LOADAVG=$val
         ;;
      meminfo)
         PLUGIN_MEMINFO=$val
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
      seti)
         PLUGIN_SETI=$val
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
   AC_CHECK_HEADERS(linux/dvb/frontend.h, [has_dvb_header=true], [has_dvb_header=false])
   if test "$has_dvb_header" = true; then
      PLUGINS="$PLUGINS plugin_dvb.o"
      AC_DEFINE(PLUGIN_DVB,1,[dvb plugin])
   else
      AC_MSG_WARN(linux/dvb/frontend.h header not found: dvb plugin disabled)
   fi   
fi
if test "$PLUGIN_EXEC" = "yes"; then
   PLUGINS="$PLUGINS plugin_exec.o"
   AC_DEFINE(PLUGIN_EXEC,1,[exec plugin])
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
   AC_CHECK_HEADERS(linux/isdn.h, [has_isdn_header=true], [has_isdn_header=false])
   if test "$has_dvb_header" = false; then
      AC_MSG_WARN(linux/isdn.h header not found: isdn plugin CPS disabled)
   fi   
   PLUGINS="$PLUGINS plugin_isdn.o"
   AC_DEFINE(PLUGIN_ISDN,1,[ISDN plugin])
fi
if test "$PLUGIN_LOADAVG" = "yes"; then
   PLUGINS="$PLUGINS plugin_loadavg.o"
   AC_DEFINE(PLUGIN_LOADAVG,1,[loadavg plugin])
fi
if test "$PLUGIN_MEMINFO" = "yes"; then
   PLUGINS="$PLUGINS plugin_meminfo.o"
   AC_DEFINE(PLUGIN_MEMINFO,1,[meminfo plugin])
fi
if test "$PLUGIN_MYSQL" = "yes"; then
   AC_CHECK_HEADERS(mysql/mysql.h, [has_mysql_header=true], [has_mysql_header=false])
   if test "$has_mysql_header" = true; then	
      AC_CHECK_LIB(mysqlclient, mysql_init, [has_mysql_lib=true], [has_mysql_lib=false])
      if test "$has_mysql_lib" = true; then
        PLUGINS="$PLUGINS plugin_mysql.o"
        AC_DEFINE(PLUGIN_MYSQL,1,[mysql plugin])
        PLUGINLIBS="$PLUGINLIBS -lmysqlclient"
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
   AC_CHECK_HEADERS(net/if_ppp.h, [has_ppp_header=true], [has_ppp_header=false])
   if test "$has_ppp_header" = true; then
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
if test "$PLUGIN_SETI" = "yes"; then
   PLUGINS="$PLUGINS plugin_seti.o"
   AC_DEFINE(PLUGIN_SETI,1,[seti plugin])
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
