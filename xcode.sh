#!/bin/bash

INITIALDIR=${PWD}
# echo "Script executed from: ${INITIALDIR}"

# BASH_SOURCE has the script's path
# could be absolute, could be relative
SCRIPT_PATH=$(dirname ${BASH_SOURCE})

FIRSTCHAR=${SCRIPT_PATH:0:1}
if [ ${FIRSTCHAR} == "/" ]; then
  # it's asolute path
  AL_LIB_PATH=${SCRIPT_PATH}
else
  # SCRIPT_PATH was relative
  AL_LIB_PATH=${INITIALDIR}/${SCRIPT_PATH}
fi

# first build allolib ###########################################################
echo " "
echo "___ building allolib __________"
echo " "
cd ${AL_LIB_PATH}
git submodule init
git submodule update
mkdir -p build
cd build
mkdir -p Debug
cd Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make
LIB_DEBUG_BUILD_RESULT=$?
# if lib failed to build, exit
if [ ${LIB_DEBUG_BUILD_RESULT} != 0 ]; then
	exit 1
fi

cd ..
mkdir -p Release
cd Release
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make
LIB_RELEASE_BUILD_RESULT=$?
# if lib failed to build, exit
if [ ${LIB_RELEASE_BUILD_RESULT} != 0 ]; then
  exit 1
fi


# then build the app ###########################################################
APP_NAME="$1" # first argument (assumming we consumed all the options above)

if [ ${APP_NAME} == "." ]; then
  # if '.' was given for the app directory,
  # it means script was run from app path
  # so initial dir has the app path
  # and it's basename will be app name
  APP_NAME=${INITIALDIR}
  APP_PATH=${APP_NAME}
else
  APP_PATH=${INITIALDIR}/${APP_NAME}
fi

# discard '/'' in the end of the input directory (if it has)
LASTCHAR=${APP_NAME:(-1)}
if [ ${LASTCHAR} == "/" ]; then
    # ${string%substring}
    # Strips shortest match of $substring from back of $string.
    APP_NAME=${APP_NAME%/}
fi

# get only last foldername
APP_NAME=$(basename ${APP_NAME})

echo " "
echo "___ generating ${APP_NAME} xCode project __________"
echo " "

echo "app path: ${APP_PATH}"
cd ${APP_PATH}
mkdir -p xcode
cd xcode
# if app is run with this script, al_path is set here
# if this script was not used, cmake will set it to value user provided
cmake ${APP_PATH}/ -Dal_path=${AL_LIB_PATH} -G Xcode