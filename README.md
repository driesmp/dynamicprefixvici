# Building
Before running build.sh, make sure strongswan is installed with the libvici.h.
The following command should do the trick: *pkg install strongswan*


# Usage
Add something like this to */usr/local/etc/dhcpcd.exit-hook*

case "$reason" in  
BOUND6|RENEW6|REBIND6|REBOOT6|INFORM6)  
&nbsp;&nbsp;&nbsp;&nbsp;prefix=`printenv new_dhcp6_ia_pd1_prefix1`  
&nbsp;&nbsp;&nbsp;&nbsp;/usr/local/sbin/dynamicprefixvici -p "$prefix" -n "global-ipv6"  
esac