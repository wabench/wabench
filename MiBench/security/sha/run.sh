#!/bin/bash

Native=./sha

NativeArg="input_large.asc"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=false

. ../../../common.sh
