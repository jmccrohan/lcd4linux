#! /bin/sh
#
# lcd4linux
#
PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/bin/lcd4linux
NAME=lcd4linux
DESC=lcd4linux

test -f $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
	start-stop-daemon --start --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
	start-stop-daemon --oknodo --stop --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "$NAME."
	;;
  reload)
	start-stop-daemon --stop --signal 1 --quiet --pidfile \
		/var/run/$NAME.pid --exec $DAEMON
  	;;
  restart|force-reload)
	echo -n "Restarting $DESC: "
	start-stop-daemon --stop --quiet --pidfile \
		/var/run/$NAME.pid --exec $DAEMON
	sleep 1
	start-stop-daemon --start --quiet --pidfile \
		/var/run/$NAME.pid --exec $DAEMON
	echo "$NAME."
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	exit 1
	;;
esac

exit 0
