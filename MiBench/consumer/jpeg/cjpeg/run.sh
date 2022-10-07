#!/bin/bash

Native=./cjpeg

NativeArg="-dct int -progressive -opt -outfile output_large_encode.jpeg input_large.ppm"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=false

. ../../../../common.sh
