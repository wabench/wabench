#!/bin/bash

Native=./rawcaudio

NativeArg="< large.pcm"

Iter=1

WasmDir=.

RunAOT=false

# Do not check result due to differences
CheckResult=true

. ../../../../common.sh
