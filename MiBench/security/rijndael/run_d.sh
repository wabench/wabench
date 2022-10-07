#!/bin/bash

Native=./rijndael

NativeArg="input_large.enc output_large.dec d 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../common.sh
