#!/bin/bash

cp mods/* ../../kicad-library/mods/
kicad-split --yes -i stm32-wifi-door-bell.lib -o ../../kicad-library/libs/
