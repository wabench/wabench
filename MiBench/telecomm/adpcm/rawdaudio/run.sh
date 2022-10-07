#!/bin/bash

Native=./rawdaudio

NativeArg="< large.adpcm"

Iter=10

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../../common.sh
