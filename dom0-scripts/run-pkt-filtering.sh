
single=$1
machine=$2
expt_no=$3
dir=$4
prefix=$5
thisVM_ip=$6
otherVM_ip=$7
duration=$8
VM_no=$9

shift 9

VM_name=$1

#Doing the same as that was done in domU earlier, simply moved the libpcap functionality into dom0 with promiscuous mode.
#The file names are retained as before, so that log-analysis scripts need not change again. So, here "self" is still the
#VM for which this instance is supposed to be running, though not running within that VM

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

func_get_xenmon_dom_number()
{
	if [ $# != 2 ]
	then
	    echo "Usage: $0 <dom-name> <xmlist-file>"
	    exit
	fi

	dom=$1
	filename=$2

	if cat $filename | grep $dom > /dev/null
	then
	    cat $filename | grep $dom | awk '{print $2}'
	else
	    echo "$0 No such domain $dom in xmlist $filename"
	fi
}

xend start
xm list > temp_xmlist_$VM_no

dom_num=`func_get_xenmon_dom_number $VM_name temp_xmlist_"$VM_no"`

echo "Starting packet filtering for $thisVM_ip"
./report_traff_per_interval "vif"$dom_num".0" 1 "$thisVM_ip" "$otherVM_ip" /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-"$self"-libpcap-"$expt_no"number-"$machine"-"$single".log 1

sleep $duration

#Directly killing here itself, instead of invoking kill-pkt-filtering.sh
killall report_traff_per_interval	
echo "Killed libpcap for $VM_no"
