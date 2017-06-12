#!/bin/sh

if [ $# -ne 5 -a $# -ne 6 ]
then
	echo "Usage: $0 <number-of-inputs-needed> <firstVM-file> <secondVM-file> <EXTVM1-file> <EXTVM2-file>"
	echo OR
	echo "Usage: $0 <number-of-inputs-needed> <firstVM-file> <secondVM-file> <EXTVM1-file> <EXTVM2-file> <create-new-inputs-flag>"
	exit
fi

numbers_in_batch=$1
firstVM_file="$2"
secondVM_file="$3"
EXTVM1_file="$4"
EXTVM2_file="$5"

if [ $# -eq 6 ]
then
	if [ $6 -eq 1 ]
	then
		#To create new inputs, delete the current files
		echo "Deleting previous input config lines in $firstVM_file $secondVM_file $EXTVM1_file $EXTVM2_file, after sleeping for 5"
		sleep 5
		rm $firstVM_file $secondVM_file $EXTVM1_file $EXTVM2_file
	else
		echo "New config lines will be appended to the files $firstVM_file $secondVM_file $EXTVM1_file $EXTVM2_file"
	fi
fi

for i in `seq 1 $numbers_in_batch`
do
	#Expt line in firstVM_file
	for load_type in 'A' 'N' 'R' 'W' 'C'
	do
		case $load_type	in
		'A') A_random=`gen-random.sh 95`
		   A_input=`expr $A_random \* 1000`
		   A_line="N $A_input 128 192.168.112.202"
			;;
		'C') C_random=`gen-random.sh 90`
			 C_input=$C_random
			 C_line="C $C_input xxx xxx"
			;;
		'N') N_random=`gen-random.sh 95`
		   N_input=`expr $N_random \* 1000`
		   N_line="N $N_input 256 192.168.112.203"
			;;
		'R') R_random=`gen-random.sh 13`	#Will give number from [0-12]
			if [ $R_random -ne 0 ]			#Removing aside the original 0
			then 
			   odd_flag=`expr $R_random % 2`
			   #For firstVM, if the random number is odd, then subtract 1 else stay
			   if [ $odd_flag -eq 1 ]
			   then
					R_input=`expr $R_random - 1`	#If R_input = 0, then power = 1
			   else
					R_input=`expr $R_random + 0`
			   fi
			   R_input=`get_power_of_2.sh $R_input`
			else
			   R_input=0	#So, no disk read load
			fi
		   #Different file prefixes for each file size
		   case $R_input in
			0) R_prefix="readdummy"
				;;
			1) R_prefix="readh"
				;;
			4) R_prefix="readg"
				;;
			16) R_prefix="reada"
				;;
			64) R_prefix="readb"
				;;
			256) R_prefix="readc"
				;;
			1024) R_prefix="readd"
				;;
			4096) R_prefix="reade"
				;;
			16384) R_prefix="readf"
				;;
		   esac
		   R_line="R 1 $R_input $R_prefix"
			;;
			'W') W_random=`gen-random.sh 13`
				if [ $W_random -eq 0 ]
				then 
					W_input=0		#So, no disk write load
				else
				   W_input=`expr $W_random - 1`
				   W_input=`get_power_of_2.sh $W_input`
				fi
				#Different file prefixes for each file size
				case $W_input in
				0) W_prefix="writedummy"
					;;
				1) W_prefix="writeh"
					;;
				4) W_prefix="writeg"
					;;
				16) W_prefix="writea"
					;;
				64) W_prefix="writeb"
					;;
				256) W_prefix="writec"
					;;
				1024) W_prefix="writed"
					;;
				4096) W_prefix="writee"
					;;
				16384) W_prefix="writef"
					;;
				0) W_prefix="writedummy"
					;;
				2) W_prefix="writes"
					;;
				8) W_prefix="writet"
					;;
	            32) W_prefix="writeu"
	                ;;
	            128) W_prefix="writev"
	                ;;
	            512) W_prefix="writew"
	                ;;
	            2048) W_prefix="writex"
	                ;;
	            8192) W_prefix="writey"
	                ;;
	            32768) W_prefix="writez"
	                ;;
				esac

		   W_line="W 1 $W_input $W_prefix"
			;;
		esac
	done
	echo $C_line $A_line $N_line $R_line $W_line >> $firstVM_file
		

	#Expt line in secondVM_file
	for load_type in 'A' 'N' 'R' 'W' 'C'
	do
		case $load_type	in
		'A') A_random=`gen-random.sh 95`
		   A_input=`expr $A_random \* 1000`
		   A_line="N $A_input 128 192.168.112.201"
			;;
		'C') C_random=`gen-random.sh 90`
			C_input=$C_random
			C_line="C $C_input xxx xxx"
			;;
		'N') N_random=`gen-random.sh 95`
		   N_input=`expr $N_random \* 1000`
		   N_line="N $N_input 256 192.168.112.204"
			;;
		'R') R_random=`gen-random.sh 13`
			if [ $R_random -ne 0 ]
			then 
				   odd_flag=`expr $R_random % 2`
				   #For secondVM, if the random number is odd, then stay, else subtract 1
				   if [ $odd_flag -eq 1 ]
				   then
						R_input=`expr $R_random + 0`
				   else
						R_input=`expr $R_random - 3`
				   fi
				   R_input=`get_power_of_2.sh $R_input`
			else
				R_input=0
			fi
		   #Different file prefixes for each file size
		   case $R_input in
			0) R_prefix="readdummy"
				;;
			2) R_prefix="reads"
				;;
			8) R_prefix="readt"
				;;
			32) R_prefix="readu"
				;;
			128) R_prefix="readv"
				;;
			512) R_prefix="readw"
				;;
			2048) R_prefix="readx"
				;;
			8192) R_prefix="ready"
				;;
			32768) R_prefix="readz"
				;;
		   esac
		   R_line="R 1 $R_input $R_prefix"
			;;
		'W') W_random=`gen-random.sh 13`
			if [ $W_random -eq 0 ]
			then 
				W_input=0		#So, no disk write load
			else
		   		W_input=`expr $W_random - 1`
	   		    W_input=`get_power_of_2.sh $W_input`
			fi
		   #Different file prefixes for each file size
		   case $W_input in
			0) W_prefix="writedummy"
				;;
            2) W_prefix="writes"
                ;;
            8) W_prefix="writet"
                ;;
            32) W_prefix="writeu"
                ;;
            128) W_prefix="writev"
                ;;
            512) W_prefix="writew"
                ;;
            2048) W_prefix="writex"
                ;;
            8192) W_prefix="writey"
                ;;
            32768) W_prefix="writez"
                ;;
			0) W_prefix="writedummy"
				;;
            1) W_prefix="writeh"
                ;;
            4) W_prefix="writeg"
                ;;
			16) W_prefix="writea"
				;;
			64) W_prefix="writeb"
				;;
			256) W_prefix="writec"
				;;
			1024) W_prefix="writed"
				;;
			4096) W_prefix="writee"
				;;
			16384) W_prefix="writef"
				;;
           esac

           W_line="W 1 $W_input $W_prefix"
            ;;
		esac
	done
	echo $C_line $A_line $N_line $R_line $W_line >> $secondVM_file


	#Expt line in EXTVM1_file
	N_random=`gen-random.sh 95`
	N_input=`expr $N_random \* 1000`
	N_line="N $N_input 256 192.168.112.201"
	echo $N_line >> $EXTVM1_file

	#Expt line in EXTVM2_file
	N_random=`gen-random.sh 95`
	N_input=`expr $N_random \* 1000`
	N_line="N $N_input 256 192.168.112.202"
	echo $N_line >> $EXTVM2_file

done
