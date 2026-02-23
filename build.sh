#!/bin/sh

set -xe

PATHS="-I./include -./Llib"

GLOBAL_ARGS="main.c ./glad.c"

LIBS="-lSDL3 -lm"

gcc -o build/main $GLOBAL_ARGS $PATHS $LIBS -DARCH_LINUX

LIBS="-lSDL3w -lwinmm -lgdi32 -luser32 -lole32 -luuid -lsetupapi -limm32 -lversion -loleaut32"

x86_64-w64-mingw32-gcc -o build/main.exe $GLOBAL_ARGS $PATHS $LIBS -DARCH_WINDOWS
