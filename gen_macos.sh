#!/bin/bash
cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Debug -S . -B ./build/DebugOSX -G Xcode
