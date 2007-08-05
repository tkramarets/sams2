#!/bin/sh -e
### BEGIN INIT INFO
# Provides:		sams
# Required-Start:	$local_fs $network $time
# Required-Stop:	$local_fs $network $time
# Default-Start:	2 3 4 5
# Default-Stop:		0 1 6
# Short-Descrition:	Starting sams daemon
# Description:		Squid Account Management System (SAMS)
#  Starting sams management daemon - samsdaemon
### END INIT INFO
#
# Author:	Pavel Vinogradov <Pavel.Vinogradov@nixdev.net>
#

SAMSPATH=`cat /etc/sams.conf | grep SAMSPATH | tr "SAMSPATH=" "\0"`
DAEMON=$SAMSPATH/bin/samsdaemon
LOCKFILE=/var/lock/samsd
PIDFILE=/var/run/samsdaemon.pid
RETVAL=0

start()
{
	echo -n "Starting samsd: "
	start-stop-daemon --start --quiet --exec $DAEMON
	RETVAL=$?
	[ $RETVAL -eq 0 ] && touch "$LOCKFILE"
        echo
}

stop()
{
	echo -n "Shutting down samsd: "
	start-stop-daemon --stop --quiet --pidfile $PIDFILE
	RETVAL=$?
	[ $RETVAL -eq 0 ] && rm -f "$LOCKFILE"
        echo
}

restart()
{
	stop
	start
}

# See how we were called.
case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		restart
		;;
	status)
	        status "$DAEMON"
		;;
	*)
		echo "Usage: ${0##*/} {start|stop|restart}"
		RETVAL=1
esac

exit $RETVAL