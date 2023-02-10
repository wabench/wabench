#!/bin/bash

RunAOT=false

MeasureMem=false

MeasurePerf=false

BenchRoot="$HOME/WABench"

CommonScript="$BenchRoot/common.sh"

BenchSize=5
BenchSuite=()
# Structure:  Benchmark directory                             Native           NativeArg         Iter  WasmDir
BenchSuite+=("JetStream2/gcc-loops"                         "./gcc-loops"      ""                "1"    "")
BenchSuite+=("JetStream2/hashset"                           "./hashset"        ""                "1"    "")
BenchSuite+=("JetStream2/quicksort"                         "./quicksort"      ""                "10"    "")
BenchSuite+=("JetStream2/tsf"                               "./tsf"            "10000"           "1"    ".")
BenchSuite+=("MiBench/automotive/basicmath"                 "./basicmath"      ""                "1"    "")
BenchSuite+=("MiBench/automotive/bitcount"                  "./bitcount"       "1125000"         "1"    "")
BenchSuite+=("MiBench/consumer/jpeg/cjpeg"                  "./cjpeg"      \
             "-dct int -progressive -opt -outfile output_large_encode.jpeg input_large.ppm" "1" ".")
BenchSuite+=("MiBench/consumer/jpeg/djpeg"                  "./djpeg"      \
             "-dct int -ppm -outfile output_large_decode.ppm input_large.jpg" "1" ".")
BenchSuite+=("MiBench/office/stringsearch"                  "./stringsearch"   ""          "100"  "")
BenchSuite+=("MiBench/security/blowfish"                    "./blowfish"   \
             "e input_large.asc output_large.enc 1234567890abcdeffedcba0987654321" "10"  ".")
BenchSuite+=("MiBench/security/blowfish"                    "./blowfish"   \
             "d input_large.enc output_large.asc 1234567890abcdeffedcba0987654321" "10"  ".")
BenchSuite+=("MiBench/security/rijndael"                    "./rijndael"   \
             "input_large.asc output_large.enc e 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321" "10"  ".")
BenchSuite+=("MiBench/security/rijndael"                    "./rijndael"   \
             "input_large.enc output_large.dec d 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321" "10"  ".")
BenchSuite+=("MiBench/security/sha"                         "./sha"            "input_large.asc" "10"   ".")
BenchSuite+=("MiBench/telecomm/adpcm/rawcaudio"             "./rawcaudio"      "< large.pcm"     "10"   ".")
BenchSuite+=("MiBench/telecomm/adpcm/rawdaudio"             "./rawdaudio"      "< large.adpcm"   "10"   ".")
BenchSuite+=("MiBench/telecomm/crc32"                       "./crc32"          "large.pcm"       "10"   ".")
BenchSuite+=("PolyBench/datamining/correlation"             "./correlation"    ""                "1"    "")
BenchSuite+=("PolyBench/datamining/covariance"              "./covariance"     ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/blas/gemm"           "./gemm"           ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/blas/gemver"         "./gemver"         ""                "10"   "")
BenchSuite+=("PolyBench/linear-algebra/blas/gesummv"        "./gesummv"        ""                "10"   "")
BenchSuite+=("PolyBench/linear-algebra/blas/symm"           "./symm"           ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/blas/syr2k"          "./syr2k"          ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/blas/syrk"           "./syrk"           ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/blas/trmm"           "./trmm"           ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/kernels/2mm"         "./2mm"            ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/kernels/3mm"         "./3mm"            ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/kernels/atax"        "./atax"           ""                "1"    "")
BenchSuite+=("PolyBench/linear-algebra/kernels/bicg"        "./bicg"           ""                "10"   "")
BenchSuite+=("PolyBench/linear-algebra/kernels/doitgen"     "./doitgen"        ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/kernels/mvt"         "./mvt"            ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/solvers/cholesky"    "./cholesky"       ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/solvers/durbin"      "./durbin"         ""                "10"  "")
BenchSuite+=("PolyBench/linear-algebra/solvers/gramschmidt" "./gramschmidt"    ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/solvers/lu"          "./lu"             ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/solvers/ludcmp"      "./ludcmp"         ""                "1"   "")
BenchSuite+=("PolyBench/linear-algebra/solvers/trisolv"     "./trisolv"        ""                "10"  "")
BenchSuite+=("PolyBench/medley/deriche"                     "./deriche"        ""                "1"   "")
BenchSuite+=("PolyBench/medley/floyd-warshall"              "./floyd-warshall" ""                "1"   "")
BenchSuite+=("PolyBench/medley/nussinov"                    "./nussinov"       ""                "1"   "")
BenchSuite+=("PolyBench/stencils/adi"                       "./adi"            ""                "1"   "")
BenchSuite+=("PolyBench/stencils/fdtd-2d"                   "./fdtd-2d"        ""                "1"   "")
BenchSuite+=("PolyBench/stencils/heat-3d"                   "./heat-3d"        ""                "1"   "")
BenchSuite+=("PolyBench/stencils/jacobi-1d"                 "./jacobi-1d"      ""                "10"  "")
BenchSuite+=("PolyBench/stencils/jacobi-2d"                 "./jacobi-2d"      ""                "1"   "")
BenchSuite+=("PolyBench/stencils/seidel-2d"                 "./seidel-2d"      ""                                        "1"   "")
BenchSuite+=("Benchmarks/bzip2"                             "./bzip2"          "-k -f -z input_file"                     "1"   ".")
BenchSuite+=("Benchmarks/espeak"                            "./espeak"         "-f input.txt -s 120 -w output_file.wav"  "1"   ".")
BenchSuite+=("Benchmarks/facedetection"                     "./facedetection"  "input.png"                               "1"   ".")
BenchSuite+=("Benchmarks/gnuchess"                          "./gnuchess"       "< input"                                 "1"   ".")
BenchSuite+=("Benchmarks/mnist"                             "./mnist"          ""                                        "1"   ".")
BenchSuite+=("Benchmarks/snappy"                            "./snappy"         ""                                        "1"   "")
BenchSuite+=("Benchmarks/whitedb"                           "./whitedb"        ""                                        "1"   "")

NumBench=$( echo "scale=0; ${#BenchSuite[@]} / $BenchSize" | bc -l )

for (( idx=0; idx<${#BenchSuite[@]}; idx+=${BenchSize} ));
do
    nth=$( echo "scale=0; $idx / $BenchSize" | bc -l)
    nth=$((nth+1))

    # For debugging
    #if [ "$nth" -ne 4 ]
    #then
    #    continue
    #fi

    echo "[${nth}/${NumBench}] ${BenchSuite[idx]}"

    # Enter benchmark directory
    cd ${BenchSuite[idx]}

    # Setup environment 
    Native=${BenchSuite[idx+1]}
    NativeArg=${BenchSuite[idx+2]}
    Iter=$( echo ${BenchSuite[idx+3]} | bc -l )
    WasmDir=${BenchSuite[idx+4]}

    # Check whether this is a dry run
    if [ "$1" != "-n" ]
    then
        # Check whether there exist binaries
        if [ ! -f "$Native" ]
        then
            echo "Building binaries..."
            make > /dev/null 2>&1
        fi
    fi

    if [ ! -f "$Native.wasm" ]
    then
        echo "Cannot build WebAssembly binary..."
        continue
    fi

    # Run benchmark
    echo "Running..."
    . $CommonScript

    # Clean up
    if [ "$1" != "-n" ]
    then
        echo "Cleanup..."
        make clean > /dev/null 2>&1
    fi

    # Enter the root directory
    cd "$BenchRoot"
    echo ""
done
