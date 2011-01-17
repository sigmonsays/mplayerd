#
# Regular cron jobs for the mplayerd package
#
0 4	* * *	root	[ -x /usr/bin/mplayerd_maintenance ] && /usr/bin/mplayerd_maintenance
