################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board/Debug" -I"C:/ti/mspm0_sdk_2_06_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_06_00_05/source" -DAXON_BOARD -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1359413199: ../gpio_toggle_output.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2030/ccs/utils/sysconfig_1.25.0/sysconfig_cli.bat" --script "C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board/gpio_toggle_output.syscfg" -o "." -s "C:/ti/mspm0_sdk_2_06_00_05/.metadata/product.json" -s "C:/ti/mspm0_sdk_2_06_00_05/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-1359413199 ../gpio_toggle_output.syscfg
device.opt: build-1359413199
device.cmd.genlibs: build-1359413199
ti_msp_dl_config.c: build-1359413199
ti_msp_dl_config.h: build-1359413199
Event.dot: build-1359413199

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board/Debug" -I"C:/ti/mspm0_sdk_2_06_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_06_00_05/source" -DAXON_BOARD -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: C:/ti/mspm0_sdk_2_06_00_05/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board/Debug" -I"C:/ti/mspm0_sdk_2_06_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_06_00_05/source" -DAXON_BOARD -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


