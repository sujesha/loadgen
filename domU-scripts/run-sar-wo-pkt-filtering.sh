#if [ $# -ne 7 ]
#then 
#	echo "Usage: $0: Insufficient number of parameters"
#	exit
##fi

#no more libpcap reporting, but VM IPs data is retained just for convenience.

single=$1
machine=$2
expt_no=$3
dir=$4
prefix=$5
thisVM_ip=$6
otherVM_ip=$7
duration=$8
VM_no=$9

echo "$0: prefix = $prefix"

mkdir -p /root/sujesha/sar-output/"$1"-core-expt/"$4"/
/etc/init.d/sysstat restart

#Earlier self was hostname, now changed to firstVM or secondVM
if [ $VM_no -eq 1 ]
then
	self="firstVM"
elif [ $VM_no -eq 2 ]
then
	self="secondVM"
else
	echo "No provision yet for more than 2 VMs"
	exit
fi

#echo "Starting iperf client on $self"
#nohup iperf -c 10.129.112.201 -i 1 -b 1M -t 5000 > /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-$self-iperf-"$3"number-"$2"-"$1".log &

echo "Starting sar on $self"
sar -bBdrRuwW -n DEV 1 5000 > /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-"$self"-sar-"$3"number-"$2"-"$1".log &

#echo "Starting mpstat on $self"
#nohup mpstat -P 0 1 5000 > /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-$self-mpstat-"$3"number-"$2"-"$1".log &
#
#echo "Starting iostat on $self"
#nohup iostat 1 5000 > /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-$self-iostat-"$3"number-"$2"-"$1".log &
#
echo "Starting top on $self"
top -b -n 5000 -d 1 > /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-"$self"-top-"$3"number-"$2"-"$1".log &

#echo "Starting packet filtering on $self"
#./report_traff_per_interval 1 "$6" "$7" /root/sujesha/sar-output/"$1"-core-expt/"$4"/"$5"-"$self"-libpcap-"$3"number-"$2"-"$1".log 1

sleep $duration

./kill-sar-wo-pkt-filtering.sh
echo "Killed logging on $self"
