#!/bin/bash
max_retry=10
retry=0
cmd="./bin/$1"
while [ $retry -lt $max_retry ]; do
  echo "Attempt $((retry+1))"
  $cmd > /dev/null
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "Command failed with exit code $ret"
    exit $ret
  fi
  retry=$((retry+1))
done

echo "Command succeeded after $retry attempts."
