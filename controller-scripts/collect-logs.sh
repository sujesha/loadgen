#!/bin/sh

if [ $# -ne 12 ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix> <resource> <repo-IP>"
        exit
fi

if [ $1 != "single" ] && [ $1 != "double" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix> <resource> <repo-IP>"
        echo "Error: First parameter should be \"single\" or \"double\""
        exit
fi

if [ $2 != "same" ] && [ $2 != "diff" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <prefix> <resource> <repo-IP>"
        echo "Error: Second parameter should be \"same\" or \"diff\""
        exit
fi

if [ $2 = "same" ]
then
        dir=`echo same-machine`
else
        dir=`echo diff-machines`
fi

echo "Collecting logs"


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
resource=$2
repo_IP="$3"

echo IPs
firstVM_ip=`cat /etc/hosts | grep $firstVM | awk '{print $1}'`
secondVM_ip=`cat /etc/hosts | grep $secondVM | awk '{print $1}'`

echo $firstVM_ip
echo $secondVM_ip


# scp the log files to $repo_IP appropriate directory
echo "Creating directory on $repo_IP - cpu-profiling-logs/"$resource"-output/$single-core-expt/"$expt_no"number/$dir"
ssh root@$repo_IP "mkdir -p cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir""

echo "scp sar-output/$single-core-expt/$dir/$prefix.xentop1-"$expt_no"number-$machine-$single.log to cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir" $repo_IP"
ssh root@$thisPM "scp /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".sar1-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xentop1-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".top1-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xenmon1-"$expt_no"number-"$machine"-"$single"* /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xmlist1-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"

        echo "scp /home/sujesha/sar-output/$single-core-expt/"$dir"/$prefix.sar2-"$expt_no"number-$machine-$single.log to $repo_IP"
ssh root@$otherPM "scp /home/sujesha/sar-output/$single-core-expt/"$dir"/"$prefix".sar2-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xentop2-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".top2-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xenmon2-"$expt_no"number-"$machine"-"$single"* /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix".xmlist2-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"

if [ $machine = "diff" ]
then
	echo "scp libpcap for $firstVM from $thisPM"
	ssh root@$thisPM "scp /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-firstVM-libpcap-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/" 
	echo "scp libpcap for $secondVM from $otherPM"
	ssh root@$otherPM "scp /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-secondVM-libpcap-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"
else
	echo "scp libpcap for both $firstVM and $secondVM from $thisPM"
	ssh root@$thisPM "scp /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-firstVM-libpcap-"$expt_no"number-"$machine"-"$single".log /home/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-secondVM-libpcap-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"
fi

echo "scp sujesha/sar-output/$single-core-expt/$dir/$prefix-firstVM-sar-"$expt_no"number-$machine-$single.log to $repo_IP"
ssh root@$firstVM "scp /root/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-firstVM-sar-"$expt_no"number-"$machine"-"$single".log /root/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-firstVM-top-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"

echo "scp sujesha/sar-output/$single-core-expt/$dir/$prefix-secondVM-sar-"$expt_no"number-$machine-$single.log to $repo_IP"
ssh root@$secondVM "scp /root/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-secondVM-sar-"$expt_no"number-"$machine"-"$single".log /root/sujesha/sar-output/"$single"-core-expt/"$dir"/"$prefix"-secondVM-top-"$expt_no"number-"$machine"-"$single".log root@$repo_IP:cpu-profiling-logs/"$resource"-output/"$single"-core-expt/"$expt_no"number/"$dir"/"

