export DIALOG_SDK_PATH=/opt/DA145xx_SDK/6.0.22.1401
# To replace docker with podman set `DOCKER=podman` in your environment
DOCKER ?= docker

# Below are some options used to debug specific functionality:
# * -DCMAKE_VERBOSE_MAKEFILE=YES
#   Debug compiler/linker flags
# * -DPRODUCTION_DEBUG_OUTPUT=YES
#   Send watchdog/hardfault info over uart
#
# Create the cmake build directory manually to add the above flags

build-release/Makefile: CMakeLists.txt arm.cmake
	mkdir -p build-release
	(cd build-release; cmake -DCMAKE_TOOLCHAIN_FILE=arm.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..)

build-debug/Makefile: CMakeLists.txt arm.cmake
	mkdir -p build-debug
	(cd build-debug; cmake -DCMAKE_TOOLCHAIN_FILE=arm.cmake -DCMAKE_BUILD_TYPE=Debug ..)

.PHONY: dockerdev
dockerdev:
	@./scripts/dockerenv.sh

.PHONY: dockerinit
dockerinit:
	${DOCKER} build -t shiftcrypto/da14531:$(shell cat .containerversion) .

.PHONY: dockerpull
dockerpull:
	${DOCKER} pull shiftcrypto/da14531

.PHONY: dockerpush
dockerpush:
	${DOCKER} build --push --platform linux/amd64,linux/arm64 -t shiftcrypto/da14531:$(shell cat .containerversion) .

.PHONY: dockerstop
dockerstop:
	@./scripts/dockerenv.sh stop

.PHONY: firmware-release
firmware-release: build-release/Makefile
	${MAKE} -C build-release bitbox-da14531-firmware

.PHONY: firmware-debug
firmware-debug: build-debug/Makefile
	${MAKE} -C build-debug bitbox-da14531-firmware

.PHONY: run
run:
	${MAKE} firmware-debug
	arm-none-eabi-gdb -x jlink.gdb build-debug/bitbox-da14531-firmware

.PHONY: gdb-server
gdb-server:
	JLinkGDBServer -device da14531 -if SWD -ir

.PHONY: rtt-client
rtt-client:
	telnet localhost 19021

.PHONY: clean
clean:
	rm -rf build-*
