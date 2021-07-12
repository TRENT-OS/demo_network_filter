#!/bin/bash -ue

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Build the analysis component.
#
# The environment variable ENABLE_ANALYSIS has to be set to ON if the build
# shall be executed with the Axivion Suite. Default for regular build is OFF.
#-------------------------------------------------------------------------------

# get the directory the script is located in
SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

# set common paths
source ${SCRIPT_DIR}/set_axivion_config


#-------------------------------------------------------------------------------
# Build the analysis component
#-------------------------------------------------------------------------------

SANDBOX_DIR="${SCRIPT_DIR}/../../../../seos_sandbox"
SOURCE_DIR="${SCRIPT_DIR}/.."

cd ${SANDBOX_DIR}

./build-system.sh ${SOURCE_DIR} zynq7000 ${BUILD_DIR} -D CMAKE_BUILD_TYPE=Debug
