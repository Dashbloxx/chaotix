#!/bin/bash

make -C tools
tools/config2macro chaotix.cfg > common/configuration.h