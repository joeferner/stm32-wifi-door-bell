#!/bin/bash

PROJECT_NAME=$(basename $(dirname $(pwd)))

arm-none-eabi-gdb -tui -ex "target extended-remote localhost:4242" build/${PROJECT_NAME}.elf
