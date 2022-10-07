#!/bin/bash

Native=./bzip2

NativeArg="-k -f -z input_file"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=false

. ../../common.sh
