#
# Nitrogen6_SoloX configuration
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.17.3)

set(LibEthdriverNumPreallocatedBuffers 32 CACHE STRING "" FORCE)

DeclareCAmkESComponents_for_NICs()
