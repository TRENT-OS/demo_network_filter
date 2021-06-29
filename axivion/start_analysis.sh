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

export ENABLE_ANALYSIS=ON

export PROJECTNAME=DemoNetworkFilterAnalysis

declare -A COMPONENTS=(
    [TimeServer]=timeServer.instance.bin
    [Ticker]=ticker.instance.bin
    [NwStack1]=nwStack1.instance.bin
    [NwStack2]=nwStack2.instance.bin
    [FilterListener]=filterListener.instance.bin
    [EntropySource]=entropySource.instance.bin
    [FilterSender]=filterSender.instance.bin
)


#-------------------------------------------------------------------------------
# Prepare workflow
#-------------------------------------------------------------------------------

ENABLE_CI_BUILD=${ENABLE_CI_BUILD:-OFF}
DEVNET_CONNECTION=${DEVNET_CONNECTION:-OFF}

# set default configuration values
export BAUHAUS_CONFIG=${AXIVION_DIR}
export AXIVION_PROJECT_DIR=${REPO_DIR}
export AXIVION_DASHBOARD_URL=http://hc-axiviondashboard:9090/axivion

LOCAL_FILESTORAGE_DIR=/home/user/filestorage
export AXIVION_DATABASES_DIR=${LOCAL_FILESTORAGE_DIR}
SERVER_FILESTORAGE_DIR=/var/filestorage
export AXIVION_SOURCESERVER_GITDIR=${SERVER_FILESTORAGE_DIR}/git/${PROJECTNAME}.git

# ensure local filestorage exists
mkdir -p ${LOCAL_FILESTORAGE_DIR}

if [[ ${ENABLE_CI_BUILD} == "ON" ]]; then

    #---------------------------------------------------------------------------
    # CI build (with devnet connection)
    #---------------------------------------------------------------------------

    echo -e "\nDo CI build (with update of dashboard server).\n"

    # mount filestorage
    sshfs filestorageuser@hc-axiviondashboard:${SERVER_FILESTORAGE_DIR} ${LOCAL_FILESTORAGE_DIR} -o idmap=user -o cache=no

else

    #---------------------------------------------------------------------------
    # Local CI build (with or without devnet connection)
    #---------------------------------------------------------------------------

    # starting local dashboard server
    dashserver start

    echo -e "\nDo local CI build (without update of dashboard server).\n"

    # use local dashboard and repo
    export AXIVION_DASHBOARD_URL=http://localhost:9090/axivion
    export AXIVION_SOURCESERVER_GITDIR=${AXIVION_PROJECT_DIR}/.git

    for COMP_NAME in "${!COMPONENTS[@]}"; do

        # check if database file already exists
        DATABASE_FILE=${LOCAL_FILESTORAGE_DIR}/${PROJECTNAME}_${COMP_NAME}.db

        if [[ ! -f "${DATABASE_FILE}" ]]; then

            # create empty database
            cidbman database create ${DATABASE_FILE}
            # install project at dashboard server
            dashserver install-project --dbfile ${DATABASE_FILE}

        fi

    done

fi


#-------------------------------------------------------------------------------
# Do analysis
#-------------------------------------------------------------------------------

# do "clean before" for the first component build
export AXIVION_CLEAN_BEFORE=true

for COMP_NAME in "${!COMPONENTS[@]}"; do

    # set environment variables for component
    export BUILD_TARGET=${COMPONENTS[${COMP_NAME}]}
    export AXIVION_PROJECTNAME=${PROJECTNAME}_${COMP_NAME}
    export AXIVION_OUTFILE=${BUILD_DIR}/os_system/${BUILD_TARGET}

    # run axivion
    axivion_ci -j

    # skip "clean before" after the first component build
    export AXIVION_CLEAN_BEFORE=false

done


#-------------------------------------------------------------------------------
# Tear down workflow
#-------------------------------------------------------------------------------

if [[ ${ENABLE_CI_BUILD} == "ON" ]]; then

    # synchronize cached writes
    sync

    # unmount filestorage
    fusermount -u ${LOCAL_FILESTORAGE_DIR}

fi
