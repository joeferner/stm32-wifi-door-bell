#!/bin/bash

PROJECT_NAME=$(basename $(dirname $(pwd)))

st-flash --reset write build/${PROJECT_NAME}.bin 0x8000000
