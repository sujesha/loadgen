#!/bin/sh
#Generic wrapper for the generic load generator

if [ $# -ne 14 -a $# -ne 20 ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP>"
		echo OR
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP> <ext-VM1> <ext-PM1-IP> <ext-VM2> <ext-PM2-IP> <ext-VM1-input-file> <ext-VM2-input-file>"
	echo "<generate-load-input-files> should contain input for the program generate_loads. "
	echo
        exit
fi

if [ $1 != "single" ] && [ $1 != "double" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP>"
		echo OR
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP> <ext-VM1> <ext-PM1-IP> <ext-VM2> <ext-PM2-IP> <ext-VM1-input-file> <ext-VM2-input-file>"
        echo "Error: First parameter should be \"single\" or \"double\""
        exit
fi

if [ $2 != "same" ] && [ $2 != "diff" ]
then
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP>"
		echo OR
        echo "Usage: $0 <[single|double]> <[same|diff]> <expt-no> <total-duration> <expt-id-text> <first-domU> <second-domU> <this-PM> <other-PM> <generate-load-input-file1> <generate-load-input-file2> <buffer-time> <resource> <repo-IP> <ext-VM1> <ext-PM1-IP> <ext-VM2> <ext-PM2-IP> <ext-VM1-input-file> <ext-VM2-input-file>"
        echo "Error: Second parameter should be \"same\" or \"diff\""
        exit
fi

if [ $# -eq 20 ]
then
    nonaff_flag=1
else
    nonaff_flag=0
fi


if [ $2 = "same" ]
then
        dir=`echo same-machine`
else
        dir=`echo diff-machines`
fi

mondd=`date|awk '{print $2$3}'`
#prefix=`echo $mondd-$5`
prefix="Aug10-$5"
echo $prefix


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

input_file_firstVM=$1
input_file_secondVM=$2
buffer_time=$3
resource=$4		#rdonly, wronly, affonly, and cpuonly, disk-rdonly, disk-wronly, random-combo
repo_IP=$5

if [ $nonaff_flag -eq 1 ]
then
	shift 5
    EXT_VM1=$1
    EXT_PM1="$2"
    EXT_VM2=$3
    EXT_PM2="$4"
    input_file_EXT_VM1=$5
    input_file_EXT_VM2=$6
fi
	
echo IPs
firstVM_ip=`cat /etc/hosts | grep $firstVM | awk '{print $1}'`
secondVM_ip=`cat /etc/hosts | grep $secondVM | awk '{print $1}'`
if [ $nonaff_flag -eq 1 ]
then
	EXT_VM1_ip=`cat /etc/hosts | grep $EXT_VM1 | awk '{print $1}'`
	EXT_VM2_ip=`cat /etc/hosts | grep $EXT_VM2 | awk '{print $1}'`
fi

arg_array_firstVM=`cat $input_file_firstVM`
arg_array_secondVM=`cat $input_file_secondVM`
if [ $nonaff_flag -eq 1 ]
then
	arg_array_EXT_VM1=`cat $input_file_EXT_VM1`
	arg_array_EXT_VM2=`cat $input_file_EXT_VM2`
fi

load_dur=`expr $duration_secs + $buffer_time \* 2`
echo load duration = $load_dur


################################################################# REMOVING RESIDUES
echo "Removing residues"
ssh root@$thisPM "./kill-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" 2> /dev/null"
if [ $machine = "diff" ]
then
        ssh root@$otherPM "./kill-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$prefix" 2> /dev/null"
fi
ssh root@$firstVM "./kill-sar-and-pkt-filtering.sh 2> /dev/null"
ssh root@$secondVM "./kill-sar-and-pkt-filtering.sh 2> /dev/null"
echo "Killing load on $firstVM"
ssh root@$firstVM "./kill-generate-load.sh"
echo "Killing load on $secondVM"
ssh root@$secondVM "./kill-generate-load.sh"
if [ $nonaff_flag -eq 1 ]
then
	ssh root@$EXT_VM1 "./kill-generate-load.sh"
	ssh root@$EXT_VM2 "./kill-generate-load.sh"
fi
######################################################################  REMOVED RESIDUES
######################################################################  PREPARING FOR LOADING (in foreground itself)
ssh-and-prep-for-generate-load.sh "$firstVM_ip" "$load_dur $arg_array_firstVM"
ssh-and-prep-for-generate-load.sh "$secondVM_ip" "$load_dur $arg_array_secondVM"
if [ $nonaff_flag -eq 1 ]
then
	ssh-and-prep-for-generate-load.sh "$EXT_VM1_ip" "$load_dur $arg_array_EXT_VM1"
	ssh-and-prep-for-generate-load.sh "$EXT_VM2_ip" "$load_dur $arg_array_EXT_VM2"
fi		#Ignore this red colour font, it is not an error. It is only happening because of the "for" term in the program's name above.
######################################################################  PREPARED FOR LOADING

###################################################################### START LOADING
ssh-and-generate-load.sh "$firstVM_ip" "$load_dur $arg_array_firstVM" &
ssh-and-generate-load.sh "$secondVM_ip" "$load_dur $arg_array_secondVM" &
if [ $nonaff_flag -eq 1 ]
then
ssh-and-generate-load.sh "$EXT_VM1_ip" "$load_dur $arg_array_EXT_VM1" &
ssh-and-generate-load.sh "$EXT_VM2_ip" "$load_dur $arg_array_EXT_VM2" &
fi
#################################################################### STARTED  LOAD
echo "sleep for buffer $buffer_time: experiment number $expt_no $machine-diff case"
sleep $buffer_time

#################################################################### START LOGGING
start-off-logs.sh $single $machine $expt_no $duration_secs $expt_id $firstVM $secondVM $thisPM $otherPM $prefix
################################################################### STARTED LOGGING
################################################################### EXPERIMENT RUN for DURATION
echo "done, now sleep $duration_secs"
sleep $duration_secs
echo "done sleep"
################################################################### DURATION ENDS
echo "sleep for buffer $buffer_time: experiment number $expt_no $machine-diff case"
sleep $buffer_time
################################################################## END LOGGING
#echo "Finishing up"
#ssh root@$thisPM "./kill-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$dir" "$prefix" 2> /dev/null"
#if [ $machine = "diff" ]
#then
#        ssh root@$otherPM "./kill-sar-with-timestamp.sh "$single" "$machine" "$expt_no" "$prefix" 2> /dev/null"
#fi
#ssh root@$firstVM "./kill-sar-and-pkt-filtering.sh 2> /dev/null"
#ssh root@$secondVM "./kill-sar-and-pkt-filtering.sh 2> /dev/null"
################################################################# ENDED LOGGING
################################################################# END LOADING
echo "Killing load on $firstVM"
ssh root@$firstVM "./kill-generate-load.sh"
echo "Killing load on $secondVM"
ssh root@$secondVM "./kill-generate-load.sh"
if [ $nonaff_flag -eq 1 ]
then
	echo "Killing load on $EXT_VM1"
	ssh root@$EXT_VM1 "./kill-generate-load.sh"
	echo "Killing load on $EXT_VM2"
	ssh root@$EXT_VM2 "./kill-generate-load.sh"
fi
###################################################################### ENDED LOADING
################################################################# END LOGGING
# scp the log files to $repo_IP appropriate directory
collect-logs.sh $single $machine $expt_no $duration_secs $expt_id $firstVM $secondVM $thisPM $otherPM $prefix $resource $repo_IP
###################################################################### ENDED LOGGING
