#!/bin/sh
#Run this from /root

apt-get update

#Xen dependencies
apt-get install build-essential make binutils zlib1g-dev python-dev libncurses-dev libssl-dev libx11-dev bridge-utils iproute udev
apt-get install gettext patch mercurial gawk texinfo python python-xml python-pyopenssl python-pam

echo "Xen installation better be done, manually, coz the procedure could vary"
echo "If Hardy with xen 3.3, just do "apt-get install ubuntu-xen-server""
echo "sleeping 5 in case xen installation hasnt been done yet, then break here"

apt-get remove network-manager

#NFS client setup
apt-get install portmap nfs-common
mkdir /home/sujesha/vm-images
chown sujesha:sujesha /home/sujesha/vm-images
mkdir /home/sujesha/vm-cfg-files
chown sujesha:sujesha /home/sujesha/vm-cfg-files
echo "192.168.250.2:/NFSDIR/vm-images /home/sujesha/vm-images nfs defaults 0 0" >> /etc/fstab
echo "192.168.250.2:/NFSDIR/vm-cfg-files /home/sujesha/vm-images nfs defaults 0 0" >> /etc/fstab

#NTP setup
apt-get install ntp ntpdate
sed -i 's/UTC=yes/UTC=no/' /etc/default/rcS
mv /etc/localtime /etc/localtime.old
ln -s /usr/share/zoneinfo/Asia/Calcutta /etc/localtime
sed -i 's/server ntp.ubuntu.com/server 192.168.250.2/' /etc/ntp.conf

/etc/init.d/ntp stop
ntpdate ntp.iitb.ac.in
/etc/init.d/ntp start

#Xen independent clock
echo "xen.independent_wallclock = 1" >> /etc/sysctl.conf

apt-get install sysstat
sed -i 's/ENABLED="false"/ENABLED="true"/' /etc/default/sysstat
/etc/init.d/sysstat restart

echo "Adding entry in NFS server remaining"
echo "/NFSDIR/vm-images/ 192.168.112.80(rw,no_subtree_check,sync)"
echo "/NFSDIR/vm-cfg-files/ 192.168.112.80(rw,no_subtree_check,sync)"



