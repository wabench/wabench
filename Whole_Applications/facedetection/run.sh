#!/bin/bash

Native=./facedetection

NativeArg="input.png"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../common.sh
