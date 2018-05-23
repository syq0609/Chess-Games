#!/bin/bash

day=$1


grep 创建房间成功 ../log/lobby-1.log.$day-* |wc -l
grep 总结算完成 ../log/lobby-1.log.$day-* |wc -l
