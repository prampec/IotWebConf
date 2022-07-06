#!/bin/bash
#
# This script will remove created PlatformIO compatible folder structure.
# Files are mostly references to original files, and the original files
#   will not be deleted. So generally said it safe to run this script.
#   platformio.ini files will be removed alog with any additional added
#   files.
#
baseDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. &> /dev/null && pwd )"
examplespio="../IotWebConf-examples"
target="${baseDir}/${examplespio}"

echo "Removing contents of ${target}"
cd ${target} || exit 1

for example in IotWebConf*; do
  echo " - ${example}"
  rm -rf ${example}/.pio
  rm -rf ${example}/lib
  rm -rf ${example} || exit $?
done

cd ..
rmdir ${target}