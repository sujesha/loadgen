#!/bin/sh

#pnum=`ps -ef | grep iperf | grep -v grep | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "                  iperf is finished on W1      "
#        echo ""
#fi

killall sar
killall top
killall report_traff_per_interval


#pnum=`ps -ef | grep sar | grep -v grep | grep -v kill | grep -v nohup| awk '{print $2}'`
#for i in `echo $pnum`
#do
#	echo "killing sar $i"
#	kill "$i";
#done
#pnum=`ps -ef | grep sar | grep -v grep | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "      Verifying: sar is finished on T1          "
#        echo ""
#fi

#pnum=`ps -ef | grep mpstat | grep -v grep | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "                  mpstat is finished                   "
#        echo ""
#fi
#
#pnum=`ps -ef | grep iostat | grep -v grep | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "                  iostat is finished                   "
#        echo ""
#fi
##

#pnum=`ps -ef | grep -v grep | grep "top -b" | grep -v kill | grep -v nohup| awk '{print $2}'`
#for i in `echo $pnum`
#do
#	echo "killing top $i"
#        kill "$i";
#done
#pnum=`ps -ef | grep -v grep | grep "top -b" | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "      Verifying: top is finished on T1          "
#        echo ""
#fi
#

#pnum=`ps -ef | grep -v grep | grep "report_traff" | grep -v kill | grep -v nohup| awk '{print $2}'`
#for i in `echo $pnum`
#do
#	echo "killing report_traffic $i"
#        kill "$i";
#done
#pnum=`ps -ef | grep -v grep | grep "report_traff" | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill "$pnum";
#else
#        echo ""
#        echo "      Verifying: report_traffic is finished on T1          "
#        echo ""
#fi

