#!/bin/bash

Native=./gnuchess

NativeArg="< input"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=false

. ../../common.sh
