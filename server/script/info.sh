#!/bin/bash

pwd
path=$(pwd)
date=$1
echo $date
echo "房间数量:"
grep '创建房间成功' $path/log/lobby-1.log.$date-* |wc -l
echo "总结算次数:"
grep '总结算完成' $path/log/lobby-1.log.$date-* |wc -l
echo "当前连接数"
netstat -natp|grep gateway_exec|grep ESTABLISHED|grep -v '127.0.0.1:6000' |grep -v ':8001' | wc -l
