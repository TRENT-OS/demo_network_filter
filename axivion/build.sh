#!/bin/bash -ue

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Build the analysis component.
#
# Usage: build.sh [sandbox_dir]
#     sandbox_dir   Folder of 'build-system.sh' (default: seos_sandbox).
#
# The environment variable ENABLE_ANALYSIS has to be set to ON if the build
# shall be executed with the Axivion Suite. Default for regular build is OFF.
#-------------------------------------------------------------------------------

# get the directory the script is located in
SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

# set common paths
source ${SCRIPT_DIR}/set_axivion_config
SOURCE_DIR="${SCRIPT_DIR}/.."


#-------------------------------------------------------------------------------
# Get arguments / show usage information
#-------------------------------------------------------------------------------

SANDBOX_DIR=${1:-"seos_sandbox"}

USAGE_INFO="Usage: $(basename $0) [sandbox_dir]
    sandbox_dir   Folder of 'build-system.sh' (default: seos_sandbox)."

if [ ! -x "${SANDBOX_DIR}/build-system.sh" ]; then
    echo "Invalid argument: SANDBOX_DIR=${SANDBOX_DIR} does not contain 'build-system.sh'."
    echo
    echo "${USAGE_INFO}"
    exit 1
fi


#-------------------------------------------------------------------------------
# Build the analysis component
#-------------------------------------------------------------------------------

cd ${SANDBOX_DIR}

./build-system.sh ${SOURCE_DIR} zynq7000 ${BUILD_DIR} -D CMAKE_BUILD_TYPE=Debug
