################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
SYSCFG_SRCS += \
../gpio_toggle_output.syscfg 

C_SRCS += \
../axon_routine.c \
../driver_config.c \
./ti_msp_dl_config.c \
C:/ti/mspm0_sdk_2_06_00_05/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c \
../isr.c \
../main.c \
../startup_mspm0g350x_ticlang.c \
../ti_msp_dl_config.c \
../uart_packet.c 

GEN_CMDS += \
./device_linker.cmd 

GEN_FILES += \
./device_linker.cmd \
./device.opt \
./ti_msp_dl_config.c 

C_DEPS += \
./axon_routine.d \
./driver_config.d \
./ti_msp_dl_config.d \
./startup_mspm0g350x_ticlang.d \
./isr.d \
./main.d \
./startup_mspm0g350x_ticlang.d \
./ti_msp_dl_config.d \
./uart_packet.d 

GEN_OPTS += \
./device.opt 

OBJS += \
./axon_routine.o \
./driver_config.o \
./ti_msp_dl_config.o \
./startup_mspm0g350x_ticlang.o \
./isr.o \
./main.o \
./uart_packet.o 

GEN_MISC_FILES += \
./device.cmd.genlibs \
./ti_msp_dl_config.h \
./Event.dot 

OBJS__QUOTED += \
"axon_routine.o" \
"driver_config.o" \
"ti_msp_dl_config.o" \
"startup_mspm0g350x_ticlang.o" \
"isr.o" \
"main.o" \
"uart_packet.o" 

GEN_MISC_FILES__QUOTED += \
"device.cmd.genlibs" \
"ti_msp_dl_config.h" \
"Event.dot" 

C_DEPS__QUOTED += \
"axon_routine.d" \
"driver_config.d" \
"ti_msp_dl_config.d" \
"startup_mspm0g350x_ticlang.d" \
"isr.d" \
"main.d" \
"startup_mspm0g350x_ticlang.d" \
"ti_msp_dl_config.d" \
"uart_packet.d" 

GEN_FILES__QUOTED += \
"device_linker.cmd" \
"device.opt" \
"ti_msp_dl_config.c" 

C_SRCS__QUOTED += \
"../axon_routine.c" \
"../driver_config.c" \
"./ti_msp_dl_config.c" \
"C:/ti/mspm0_sdk_2_06_00_05/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c" \
"../isr.c" \
"../main.c" \
"../startup_mspm0g350x_ticlang.c" \
"../ti_msp_dl_config.c" \
"../uart_packet.c" 

SYSCFG_SRCS__QUOTED += \
"../gpio_toggle_output.syscfg" 


