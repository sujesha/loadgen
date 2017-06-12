#!/bin/sh

if [ $# != 7 ]
then
	echo "Usage $0 <single|double> <same|diff> <same|diff> <dom1> <dom2> <cpu|net|disk|combo> <PM-no>"
	exit
fi

single=$1
machine=$2
core=$3
dom1=$4
dom2=$5
resource=$6
PM_no=$7

/etc/init.d/xend restart

if [ $1 != "single" ] && [ $1 != "double" ]
then
		echo "Usage $0 <single|double> <same|diff> <same|diff> <dom1> <dom2> <cpu|net|disk|combo> <PM-no>"
        echo "Error: First parameter should be \"single\" or \"double\""
        exit
fi

if [ $2 != "same" ] && [ $2 != "diff" ]
then
		echo "Usage $0 <single|double> <same|diff> <same|diff> <dom1> <dom2> <cpu|net|disk|combo> <PM-no>"
        echo "Error: Second parameter should be \"same\" or \"diff\""
        exit
fi

if [ $3 != "same" ] && [ $3 != "diff" ]
then
		echo "Usage $0 <single|double> <same|diff> <same|diff> <dom1> <dom2> <cpu|net|disk|combo> <PM-no>"
        echo "Error: Second parameter should be \"same\" or \"diff\""
        exit
fi

if [ $PM_no -eq 1 ]
then

		if [ $2 = "diff" ]
		then
			xm vcpu-set 0 1
			xm vcpu-pin 0 0 0 
			if [ $3 = "diff" ]
			then
				xm vcpu-pin "$dom1" 0 1
			else
				xm vcpu-pin "$dom1" 0 0
			fi
		else
			if [ $3 = "diff" ]
				then
				xm vcpu-set 0 2
				xm vcpu-pin 0 0 0,1
				xm vcpu-pin 0 1 0,1
                xm vcpu-pin "$dom1" 0 2
				xm vcpu-pin "$dom2" 0 3
			else
				xm vcpu-set 0 1
				xm vcpu-pin 0 0 0
						xm vcpu-pin "$dom1" 0 0
						xm vcpu-pin "$dom2" 0 0
			fi
		fi
elif [ $PM_no -eq 2 ]
then
		if [ $2 = "diff" ]
		then
				xm vcpu-set 0 1
				xm vcpu-pin 0 0 0
				if [ $3 = "diff" ]
				then
						xm vcpu-pin "$dom2" 0 1
				else
						xm vcpu-pin "$dom2" 0 0
        fi
		else
			echo "Same machine case. Do nothing."
		fi
fi

