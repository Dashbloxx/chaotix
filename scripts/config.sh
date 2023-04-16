#!/bin/bash

make -C tools
rm common/configuration_defines.h
tools/config2macro chaotix.cfg > common/configuration_defines.h