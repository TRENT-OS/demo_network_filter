#!/bin/bash -ue

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Start the analysis with the Axivion Suite activated.
#-------------------------------------------------------------------------------

# get the directory the script is located in
SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

# set common paths
source ${SCRIPT_DIR}/set_common_paths


#-------------------------------------------------------------------------------
# Prepare analysis
#-------------------------------------------------------------------------------

export PROJECTNAME=DemoNetworkFilterAnalysis

# The TARGETS variable defines the CMake build targets and outfiles for multiple
# analysis projects. Usually the targets are components of a TRENTOS system. The
# name of the analysis project will be "PROJECTNAME_TargetName".
declare -A TARGETS=(
    # [TargetName]="<BuildTarget> <OutfileInBuildDir>"
    [TimeServer]="timeServer.instance.bin os_system/timeServer.instance.bin"
    [Ticker]="ticker.instance.bin os_system/ticker.instance.bin"
    [NwStack1]="nwStack1.instance.bin os_system/nwStack1.instance.bin"
    [NwStack2]="nwStack2.instance.bin os_system/nwStack2.instance.bin"
    [FilterListener]="filterListener.instance.bin os_system/filterListener.instance.bin"
    [EntropySource]="entropySource.instance.bin os_system/entropySource.instance.bin"
    [FilterSender]="filterSender.instance.bin os_system/filterSender.instance.bin"
)


#-------------------------------------------------------------------------------
# Execute analysis
#-------------------------------------------------------------------------------

. ${AXIVION_COMMON_DIR}/execute_analysis.sh
