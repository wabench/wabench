#!/bin/bash

Native=./blowfish

NativeArg="d input_large.enc output_large.asc 1234567890abcdeffedcba0987654321"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../common.sh
