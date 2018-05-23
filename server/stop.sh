#!/bin/bash

ROOT_DIR=$(pwd)

PROCESSES_DIR=$ROOT_DIR/processes

CONFIG_DIR=$ROOT_DIR/config

LOG_DIR=$ROOT_DIR/log


PROCESS_EXIST=0
PROCESS_NOT_EXIST=1


PROCESS_LIST=("gateway" "lobby" "router")

function killProcess()
{
	RESULT=$(ps -x | egrep _exec | egrep -v 'grep'|awk '{print $5 $6}' |grep "$1")
	if [[ "$RESULT" == "" ]]
	then
		return $PROCESS_NOT_EXIST
	else
		killall $1"_exec"
		return $PROCESS_EXIST
	fi
}

for ((i=0; i<${#PROCESS_LIST[*]}; )); do
	PROCESS=${PROCESS_LIST[$i]}
	echo "send kill signal to $PROCESS"_exec" ......"
	killProcess $PROCESS
	if [[ $? -eq $PROCESS_NOT_EXIST ]]
	then
		echo -e "$PROCESS"_exec" exited!\n"
		i=$(($i+1))
	else
        sleep 1
		continue
	fi
done

