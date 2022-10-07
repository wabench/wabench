#!/bin/bash

Native=./blowfish

NativeArg="e input_large.asc output_large.enc 1234567890abcdeffedcba0987654321"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../common.sh
