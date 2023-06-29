#!/bin/bash
time=$(date "+%Y%m%d%H%M%S")
file="log_check/callgrind.out.$time"
cmd="valgrind --tool=callgrind --callgrind-out-file=$file ./bin/$1"
$cmd