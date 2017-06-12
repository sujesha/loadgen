#!/bin/sh

if [ $# -ne 10 ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix>"
        exit
fi

if [ $1 != "single" ] && [ $1 != "double" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix>"
        echo "Error: First parameter should be \"single\" or \"double\""
        exit
fi

if [ $2 != "same" ] && [ $2 != "diff" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix>"
        echo "Error: Second parameter should be \"same\" or \"diff\""
        exit
fi

if [ $2 = "same" ]
then
        dir=`echo same-machine`
else
        dir=`echo diff-machines`
fi



single=$1
machine=$2
expt_no=$3
duration_secs=$4
expt_id=$5
firstVM=$6
secondVM=$7
thisPM=$8
otherPM=$9

shift 9

prefix=$1

firstVM_ip=`cat /etc/hosts | grep $firstVM | awk '{print $1}'`
secondVM_ip=`cat /etc/hosts | grep $secondVM | awk '{print $1}'`


echo "Starting off all logging. Later run collect-logs.sh with same parameters"


echo "Starting on $thisPM"
ssh root@$thisPM "nohup ./run-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$duration_secs" "1" 2&>1 &"
echo "Starting on $otherPM"
ssh root@$otherPM "nohup ./run-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$duration_secs" "2" 2&>1 &"

if [ $machine = "diff" ]
then
	echo "Starting libpcap for "$firstVM_ip" on $thisPM"
	ssh root@$thisPM "nohup ./run-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$firstVM_ip" "$secondVM_ip" "$duration_secs" "1" "$firstVM" 2&>1 &"
	echo "Starting libpcap for "$secondVM_ip" on $otherPM"
	ssh root@$otherPM "nohup ./run-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$secondVM_ip" "$firstVM_ip" "$duration_secs" "2" "$secondVM" 2&>1 &"
else
	echo "Starting libpcap for both "$firstVM_ip" and "$secondVM_ip" on $thisPM"
	ssh root@$thisPM "nohup ./run-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$firstVM_ip" "$secondVM_ip" "$duration_secs" "1" "$firstVM" 2&>1 &"
	ssh root@$thisPM "nohup ./run-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$secondVM_ip" "$firstVM_ip" "$duration_secs" "2" "$secondVM" 2&>1 &"
fi

echo "Starting on $firstVM"
ssh root@$firstVM "nohup sh run-sar-wo-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$firstVM_ip" "$secondVM_ip" "$duration_secs" "1" 2&>1 &"

echo "Starting on $secondVM"
ssh root@$secondVM "nohup sh run-sar-wo-pkt-filtering.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" "$secondVM_ip" "$firstVM_ip" "$duration_secs" "2" 2&>1 &"
