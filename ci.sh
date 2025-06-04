#!/usr/bin/env sh
# a simple CI script

set -xe

die() {
    echo "$1"
    exit 1
}

make firmware-release -j$(($(nproc)+1))
make firmware-debug -j$(($(nproc)+1))

./scripts/print_metadata build-release/bitbox-da14531-firmware.bin

# The address of the metadata MUST NEVER change once a public release has been made.
[ "$(awk '/^ .metadata/ {print $2}' build-release/output.map)" = "0x07fc0110" ] || die ".metadata in release changed location"
[ "$(awk '/^ .metadata/ {print $2}' build-debug/output.map)" = "0x07fc0110" ] || die ".metadata in debug changed location"


# if the layout/size of the metadata changes CI needs to be updated here:
[ "$(awk '/^ .metadata/ {print $3}' build-release/output.map)" = "0x17" ] || die ".metadata in release changed size"
[ "$(awk '/^ .metadata/ {print $3}' build-debug/output.map)" = "0x17" ] || die ".metadata in debug changed size"

# -*:  disable all
# clang-analyzer-*: enable clang analyzer checks
# -clang-analyzer-cplusplus*: disable c++ checks
clang-tidy-18 -checks='-*,clang-analyzer-*,-clang-analyzer-cplusplus*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling' src/*.c

echo "success!"
