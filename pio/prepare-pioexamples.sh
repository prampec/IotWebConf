#!/bin/bash
#
# This script will create a folder structure containing PlatformIO
# compatible projects for each example under examples-pio.
#   Each project will utilize the same .ini found as prototype in this
#   folder.
# The output of this script can be added to the workspace of Visual Studio Code.
#
baseDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. &> /dev/null && pwd )"
examplespio="../IotWebConf-examples"
target="${baseDir}/${examplespio}"
test -e ${target} || mkdir ${target}

cd ${baseDir}/examples
for example in IotWebConf*; do
  if [ ! -e "${target}/$example" ]; then
    mkdir "$target/$example"
    if [ -e "$example/platformio.ini" ]; then
      for f in ${baseDir}/examples/$example/*; do
        fb="$(basename -- $f)"
        ln -s "${baseDir}/examples/$example/$fb" "$target/$example/"
      done
    else
      mkdir "$target/$example/src"
      if [ -e "$example/pio/platformio.ini" ]; then
        cp -R "$example/pio/"* "$target/$example/"
      else
        cp "${baseDir}/pio/platformio-template.ini" "$target/$example/platformio.ini"
      fi
      ln -s "${baseDir}/examples/$example/$example.ino" "$target/$example/src/main.cpp"
    fi
    mkdir "$target/$example/lib"
    ln -s "${baseDir}" "$target/$example/lib/IotWebConf"
    echo "		{"
    echo "			\"path\": \"${examplespio}/$example\""
    echo "		},"
  fi
done
