#!/bin/bash

for a in samples/*.decaf; do
	# Some files now read input from the user so they don't have a .out file
	# Don't run those in automated testing

	if [ -f ${a%.*}.out ]; then
		rm /tmp/`basename ${a%.*}.asm`;

		touch /tmp/`basename ${a%.*}.txt`;

		echo ${a%.*};

		#cat ${a%.*}.decaf | ./dcc > /tmp/`basename ${a%.*}.txt` 2>&1;
		./dcc < ${a%.*}.decaf > /tmp/`basename ${a%.*}.asm`

                cat defs.asm >> /tmp/`basename ${a%.*}.asm`

                spim -file "/tmp/`basename ${a%.*}.asm`" | tail -n +5 > /tmp/`basename ${a%.*}.txt`

		diff -y -w ${a%.*}.out /tmp/`basename ${a%.*}.txt`;
		echo
	fi
done
