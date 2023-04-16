#!/bin/bash

make -C tools
rm common/configuration.h
tools/config2macro chaotix.cfg > common/configuration.h