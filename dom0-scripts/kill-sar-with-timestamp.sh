#!/bin/sh

killall xentop-with-timestamp.pl
killall sar
killall top
killall python /usr/sbin/xenmon.py

#echo "Noting the final xmlist on desktop3"
#/etc/init.d/xend restart
#xm list > /home/piyushmasrani/sar-output/"$1"-core-expt/"$4"/"$5".xmlistend1-"$3"number-"$2"-"$1".log

if pgrep xentop-with-timestamp.pl
then
    killall xentop-with-timestamp.pl
else
    echo ""
    echo "    Verifying: xentop-with-timestamp is not running on desktop3   "
    echo ""
fi

if pgrep sar | grep -v grep > /dev/null
then
    killall sar
else
    echo ""
    echo "    Verifying: sar is not running on desktop3   "
    echo ""
fi

if pgrep top
then
    killall top
else
    echo ""
    echo "    Verifying: top is not running on desktop3   "
    echo ""
fi



#pnum=`ps -ef | grep -v grep | grep xentop-with | grep -v kill | grep -v nohup| awk '{print $2}'`
#for i in `echo $pnum`
#do
#        echo "killing xentop $i"
#        kill "$i";
#done
#pnum=`ps -ef | grep -v grep | grep xentop-with | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        sudo kill $pnum;
#else
#        echo ""
#        echo "    Verifying: xentop is finished on desktop1   "
#        echo ""
#fi
#
#
#pnum=`ps -ef | grep -v grep | grep sar | grep -v nohup | grep -v kill| awk '{print $2}'`
#for i in `echo $pnum`
#do
#	echo "killing sar $i"
#        kill "$i";
#done
#pnum=`ps -ef | grep -v grep | grep sar | grep -v nohup | grep -v kill| awk '{print $2}'`
#if [ $pnum ]
#then
#        sudo kill $pnum;
#else
#        echo ""
#        echo "    Verifying: sar is finished on desktop1   "
#        echo ""
#fi



pnum=`ps -ef | grep -v grep | grep xenmon.py | grep -v kill | grep -v nohup| awk '{print $2}'`
for i in `echo $pnum`
do
	echo "killing xenmon $i"
        kill "$i";
done
pnum=`ps -ef | grep -v grep | grep xenmon.py | grep -v kill | grep -v nohup| awk '{print $2}'`
if [ $pnum ]
then
        sudo kill $pnum;
else
        echo ""
        echo "    Verifying: xenmon is finished on desktop1   "
        echo ""
fi



#
#pnum=`ps -ef | grep -v grep | grep iostat | grep -v nohup | awk '{print $2}'`
#if [ $pnum ]
#then
#        kill $pnum;
#else
#        echo ""
#        echo "          iostat is not running on desktop1          "
#        echo ""
#fi
#
#pnum=`ps -ef | grep -v grep | grep "top -b" | grep -v xentop| awk '{print $2}'`
#for i in `echo $pnum`
#do
#	echo "killing top $i"
#        kill "$i";
#done
#pnum=`ps -ef | grep -v grep | grep "top -b" | grep -v xentop| awk '{print $2}'`
#if [ $pnum ]
#then
#        sudo kill $pnum;
#else
#        echo ""
#        echo "    Verifying: top is finished on desktop1   "
#        echo ""
#fi
#


#pnum=`ps -ef | grep -v grep | grep mpstat | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill $pnum;
#else
#        echo ""
#        echo "                  mpstat is not running on desktop1      "
#        echo ""
#fi
##
