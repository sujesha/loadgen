#!/bin/sh

killall generate_loads

#pnum=`ps -ef | grep -v grep | grep "generate_loads" | grep -v kill | grep -v nohup| awk '{print $2}'`
#if [ $pnum ]
#then
#        kill -9 "$pnum";
#else
#        echo ""
#        echo "      Verifying: generate_loads is finished       "
#        echo ""
#fi
#
