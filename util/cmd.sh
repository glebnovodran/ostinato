#!/bin/sh

PROG_NAME=ostinato
PIPE_NAME=ostinato_cmd

if [ -f $PIPE_NAME ]; then
	rm $PIPE_NAME
fi

./$PROG_NAME exeinfo pipes
if [ $? -ne 1 ]; then
	echo "Looks like cmd support wasn't compiled-in."
	echo " Try rebuilding with -DOSTINATO_USE_PIPES."
	exit 1 
fi

mkfifo $PIPE_NAME


./$PROG_NAME -meminfo:1 -w:512 -h:384 -smap:-1 -vl:1 &
sleep 3


PROG_ID="$(ps -a | grep $PROG_NAME | awk '{print $1}')"
if [ -n "$PROG_ID" ]; then
	echo "Ostinato is running with PID = $PROG_ID"
else
	echo "Ostinato is not running."
	exit 1
fi

while :; do
	printf "\e[90mcmd>\e[0m "
	read CMD
	if [ "$CMD" = "quit" ]; then
		break
 	fi
	if [ -n "CMD" ]; then
		echo -n "$CMD" > $PIPE_NAME
	fi
done

kill $PROG_ID

rm $PIPE_NAME