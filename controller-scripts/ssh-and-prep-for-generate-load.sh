#!/bin/sh 

#echo "$0: $2"

if [ $# != 2 ]
then
	echo "Usage: $0 <VM-ip> <load-parameter-list>"
	exit
fi

VM_ip=$1
arg_array_VM="$2"
daemonize="1"


echo "loading prep on $VM_ip in progress"
ssh $VM_ip "./prep_for_generate_loads "$daemonize $arg_array_VM""
echo "loading prep on $VM_ip done"
