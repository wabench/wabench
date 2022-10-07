#!/bin/bash

Native=./espeak

NativeArg="-f input.txt -s 120 -w output_file.wav"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=false

. ../../common.sh
