single=$1
machine=$2
expt_no=$3
dir=$4
prefix=$5
duration=$6
PM_no=$7


mkdir -p /home/sujesha/sar-output/"$1"-core-expt/"$4"/
/etc/init.d/sysstat restart

/etc/init.d/xend restart


echo "Noting the xmlist on PM $PM_no"
xm list > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".xmlist"$PM_no"-"$3"number-"$2"-"$1".log


echo "Starting sar on PM $PM_no"
#sar -bBdrRuwW -n DEV -P 0 -P 1 1 5000 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".sar1-"$3"number-"$2"-"$1".log &
sar -bBdrRuwW -n DEV 1 5000 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".sar"$PM_no"-"$3"number-"$2"-"$1".log &

echo "Starting xentop on PM $PM_no"
./xentop-with-timestamp.pl 1 5000 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".xentop"$PM_no"-"$3"number-"$2"-"$1".log &

#echo "Starting iostat on PM $PM_no"
#nohup iostat 1 5000 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".iostat"$PM_no"-"$3"number-"$2"-"$1".log &
#
echo "Starting top on PM $PM_no"
top -b -n 5000 -d 1 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".top"$PM_no"-"$3"number-"$2"-"$1".log &
#
#echo "Starting mpstat on PM $PM_no"
#nohup mpstat -P 0 -P 1 1 5000 > /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".mpstat"$PM_no"-"$3"number-"$2"-"$1".log &

echo "Starting xenmon on PM $PM_no"
xenmon.py -n -p /home/sujesha/sar-output/"$1"-core-expt/"$4"/"$5".xenmon"$PM_no"-"$3"number-"$2"-"$1" -t 5000 --allocated --blocked --waited --noexcount --noiocount -i 1000 &

sleep $duration

./kill-sar-with-timestamp.sh
