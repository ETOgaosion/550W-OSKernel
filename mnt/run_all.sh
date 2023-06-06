#!/bin/bash

str=$(ls)
set -- $str

for i in $str
do
	file_type=$(file -b ${i})
    if [[ "${file_type}" = "ELF 64-bit LSB executable, UCB RISC-V, version 1 (SYSV), statically linked, with debug_info, not stripped" ]]
	then
		echo "Testing $i..."
		./$i > ./$i.out
	fi
done
