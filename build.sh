#!/bin/sh

cd build

make

if [ "$?" != "0" ] ; then
  echo "Could not build!"
  exit 1
fi

make install

cd ..
