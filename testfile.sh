#!/bin/zsh

./test-file > out-file
if diff -q out-file answer-file > /dev/null; then
    echo test1: OK
else
    echo test1: NG
fi

./test-datadef >out-def
if diff -q out-def answer-def > /dev/null; then
    echo test2: OK
else
    echo test2: NG
fi

./test-datamanip >out-manip
if diff -q out-manip answer-manip > /dev/null; then
    echo test3: OK
else
    echo test3: NG
fi