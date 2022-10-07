#!/bin/bash

Native=./rijndael

NativeArg="input_large.asc output_large.enc e 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../common.sh
