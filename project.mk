###############################################################################
 #
 # Copyright (C) 2024 Analog Devices, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################
# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system

# **********************************************************

# Project name
PROJECT = max_firmware

# Enable FreeRTOS library
LIB_FREERTOS = 1

# Enable Cordio library
LIB_CORDIO = 1

# Cordio library options
RTOS = freertos
INIT_PERIPHERAL = 1
INIT_CENTRAL = 0

# TRACE option
# Set to 0 to disable
# Set to 1 to enable serial port trace messages
# Set to 2 to enable verbose messages
TRACE = 1

# **********************************************************
# Source Paths - Add all module directories
# **********************************************************

# Application sources
VPATH += app
IPATH += app

# Workout module
VPATH += workout
IPATH += workout

# Input module
VPATH += input
IPATH += input

# Communications module (BLE)
VPATH += comms
IPATH += comms

# Storage module
VPATH += storage
IPATH += storage

# RTOS module
VPATH += rtos
IPATH += rtos

# Utilities module
VPATH += utils
IPATH += utils

# **********************************************************
# Source Files
# **********************************************************

# App sources
SRCS += main.c
SRCS += app_init.c

# BLE/Comms sources
SRCS += ble_manager.c
SRCS += ble_stack.c
SRCS += svc_custom.c
SRCS += protocol.c
SRCS += ble_tx.c

# Workout sources
SRCS += workout_state.c
SRCS += workout_control.c

# Input sources
SRCS += buttons.c
SRCS += max7325.c

# Storage sources
SRCS += buffer.c

# RTOS sources
SRCS += tasks.c
SRCS += freertos_tickless.c

# Utils sources
SRCS += time_utils.c

