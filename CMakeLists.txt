# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# Use add_library() with the STATIC option to name the source file in the generated project.
# This avoids running the linker and is intended for use with cross-compiling toolchains that
# cannot link without custom flags or linker scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if(BUILD_KIT)
	message(NOTICE "Build Kit is defined as ${BUILD_KIT}")
endif()

message(NOTICE "CMAKE_CURRENT_BINARY_DIR is defined as ${CMAKE_CURRENT_BINARY_DIR}")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    message(NOTICE "Multi-config generator(${CMAKE_GENERATOR}) is active")
else()
    message(NOTICE "Single-config generator(${CMAKE_GENERATOR}) is active")
	if(DEFINED CMAKE_BUILD_TYPE)
	 	message(NOTICE "CMAKE_BUILD_TYPE is defined as ${CMAKE_BUILD_TYPE}")
	elseif(DEFINED BUILD_TYPE)
		set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
		message(NOTICE "CMAKE_BUILD_TYPE is set to BUILD_TYPE ENV ${CMAKE_BUILD_TYPE}")
	else()
		message(FATAL_ERROR "Please specify the CMAKE_BUILD_TYPE with -DCMAKE_BUILD_TYPE=")
	endif()
    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	message(NOTICE "CMAKE_RUNTIME_OUTPUT_DIRECTORY is defined as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

cmake_minimum_required(VERSION 3.16.4)

# Project configuration
project(console_rtt
	VERSION 1.0.0
	LANGUAGES C CXX ASM)

set(CMAKE_CROSSCOMPILING "TRUE")
set(CMAKE_EXPORT_COMPILE_COMMANDS "TRUE")

include(${CMAKE_CURRENT_LIST_DIR}/Config.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedCfg.cmake)

# If required user can amend RASC generated flags here, e.g. list(REMOVE_ITEM RASC_CMAKE_C_FLAGS "-Wall")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

# ─────────────────────────────────────────────────────────────────────────────
# CONSOLE_RTT integration  (added on top of the RASC-generated project)
# ─────────────────────────────────────────────────────────────────────────────
# GeneratedSrc.cmake above defines the executable target ${PROJECT_NAME}.elf
# (here: console_rtt.elf) and globs ra/, ra_gen/ and src/. We fold the console
# application, the newlib retarget PES, and the official SEGGER RTT engine
# DIRECTLY into that ELF (not as a static archive), so the strong _write()/
# _read() in rs_console_rtt_syscalls.c are always linked and override the
# newlib syscall stubs without needing "-u _write -u _read".
#
# Constraints honoured here:
#   - No UART, no FreeRTOS, no Picolibc, no logger.
#   - SEGGER_RTT_printf.c and SEGGER_RTT_Syscalls_GCC.c are deliberately NOT
#     compiled: rs_console_rtt_syscalls.c is the sole _write()/_read() provider.
#   - app/app_console_rtt/code/main.c is NOT compiled here — ra_gen/main.c owns
#     main(); src/hal_entry.c calls console_rtt_app_run().
#
# Repo layout: this file lives at
#   app/app_console_rtt/Platforms/ra2e3/fpb-ra2e3/CMakeLists.txt
# so the repository root is five levels up.
set(_repo_root  "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(_app_dir    "${_repo_root}/app/app_console_rtt")
set(_pes_dir    "${_repo_root}/pes/console_rtt")
set(_segger_dir "${_repo_root}/shared/libs/SEGGER_RTT")

target_sources(${PROJECT_NAME}.elf PRIVATE
    ${_app_dir}/code/console_rtt_app.c              # application use-case (bounded-line demo)
    ${_pes_dir}/code/src/rs_console_rtt.c           # portable RTT logic (rs_console_rtt_*_impl)
    ${_pes_dir}/code/src/rs_console_rtt_syscalls.c  # newlib _write()/_read() -> SEGGER RTT
    ${_segger_dir}/SEGGER_RTT.c                     # official SEGGER RTT V7.96e engine
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${_segger_dir}                            # SEGGER_RTT.h / SEGGER_RTT_Conf.h
)

# newlib syscall stubs: nano.specs (from RASC) provides the reduced libc; add
# nosys.specs so the syscalls we do NOT implement (_close/_lseek/_fstat/_isatty/
# _getpid/_kill/_exit ...) resolve to weak stubs. Our strong _write()/_read()
# (rs_console_rtt_syscalls.c) are compiled directly into the ELF above and
# therefore win over the nosys weak versions — no multiple-definition
# conflict, no "-u" needed.
target_link_options(${PROJECT_NAME}.elf PRIVATE --specs=nosys.specs)
