################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"syscfg/device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -Os -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src/AXON_BOARD" -I"C:/ti/mspm0_sdk_2_06_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_06_00_05/source" -DAXON_BOARD -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src/AXON_BOARD/syscfg" -std=c18 $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

soma_routine.o: ../soma_routine.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"syscfg/device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -Os -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src/AXON_BOARD" -I"C:/ti/mspm0_sdk_2_06_00_05/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_06_00_05/source" -DAXON_BOARD -gdwarf-4 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(basename\ $(<F)).o" -I"C:/DATA/DEVELOP/Git/capsule_toy_control_board/src/AXON_BOARD/syscfg" -std=c18 $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


