#!/bin/bash

for a in samples/*.frag; do
	rm /tmp/`basename ${a%.*}.txt`;
	touch /tmp/`basename ${a%.*}.txt`;

	echo ${a%.*};

	cat ${a%.*}.frag | ./dcc > /tmp/`basename ${a%.*}.txt` 2>&1;

	diff -y -w ${a%.*}.out /tmp/`basename ${a%.*}.txt`;
done
