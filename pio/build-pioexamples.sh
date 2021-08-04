#!/bin/bash
#
# This script will build all targets in pre-created PlatformIO
# compatible folder structure.
# Build will stop on any error.
#
baseDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. &> /dev/null && pwd )"
examplespio="../IotWebConf-examples"
target="${baseDir}/${examplespio}"

cd ${target} || exit 1

for example in IotWebConf*; do
  echo "Compiling ${example}"
  pio run --project-dir ${example} || exit $?
done