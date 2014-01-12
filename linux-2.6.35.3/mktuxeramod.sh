#!/bin/bash

ACCOUNT="qsi"
PASSWORD="jaga97huqew3"
PWD=`pwd`

TUXERA_CMD=$PWD/tuxera_update_ntfs.sh

if [ ! -f "$PWD/kheaders.tar.bz2" ]
then
    echo "No found headers packaage!"
    echo "Building kheaders.tar.bz2..."
$TUXERA_CMD --source-dir $PWD
    echo "Done."   
else
    echo "Found kheaders.tar.bz2!"
fi

echo "Building Tuxera exFat module..."
$TUXERA_CMD  -a --user $ACCOUNT --pass $PASSWORD --use-package $PWD/kheaders.tar.bz2
echo "Done."
echo "------------------"
echo "FINISH BUILDING."
echo "------------------"

