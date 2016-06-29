#!/bin/sh
if [ ! -n "$1" ]
then
	test="test"
	echo "no test specified, running test"
else
	echo "running $1"
	test=$1
fi

# we use g++ (not gcc) because to work with both gqt and gtk, some applications have to use c++ casts..
g++ -ggdb3 "examples/$test.c" -o test `pkg-config --cflags --libs gtk+-2.0`
./test
