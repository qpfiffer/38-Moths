#!/bin/bash

emcc -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
    -I ../include \
    ../src/{grengine,greshunkel,logging,parse,utils,vector}.c
