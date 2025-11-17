################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS

GEN_OPTS__FLAG := @"./device.opt" 
GEN_CMDS__FLAG := -Wl,-l"./device_linker.cmd" 

ORDERED_OBJS += \
"./axon_routine.o" \
"./driver_config.o" \
"./ti_msp_dl_config.o" \
"./startup_mspm0g350x_ticlang.o" \
"./isr.o" \
"./main.o" \
"./soma_uart_test.o" \
"./uart_packet.o" \
"./driver/UART.o" \
"./driver/dma/DMAMSPM0.o" \
"./driver/uart/UARTMSPM0.o" \
"./driver/utils/List.o" \
"./driver/utils/RingBuf.o" \
"./driver/utils/StructRingBuf.o" \
"./peripheral/fram_memory_map.o" \
"./peripheral/msp_peripheral_config.o" \
"./protcol/src/s2a_client.o" \
"./protcol/src/t2s_packet.o" \
"./protcol/src/target.o" \
$(GEN_CMDS__FLAG) \
-Wl,-ldevice.cmd.genlibs \
-Wl,-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include AXON_BOARD/subdir_vars.mk
-include driver/subdir_vars.mk
-include driver/dma/subdir_vars.mk
-include driver/uart/subdir_vars.mk
-include driver/utils/subdir_vars.mk
-include peripheral/subdir_vars.mk
-include protcol/src/subdir_vars.mk
-include subdir_rules.mk
-include AXON_BOARD/subdir_rules.mk
-include driver/subdir_rules.mk
-include driver/dma/subdir_rules.mk
-include driver/uart/subdir_rules.mk
-include driver/utils/subdir_rules.mk
-include peripheral/subdir_rules.mk
-include protcol/src/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
axon_board.out 

EXE_OUTPUTS__QUOTED += \
"axon_board.out" 


# All Target
all: $(OBJS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "axon_board.out"

# Tool invocations
axon_board.out: $(OBJS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: Arm Linker'
	"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -DAXON_BOARD -gdwarf-3 -Wl,-m"axon_board.map" -Wl,-i"C:/ti/mspm0_sdk_2_06_00_05/source" -Wl,-i"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board" -Wl,-i"C:/DATA/DEVELOP/Git/capsule_toy_control_board_CCS/axon_board/Debug/syscfg" -Wl,-i"C:/ti/ccs2030/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/lib" -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="axon_board_linkInfo.xml" -Wl,--rom_model -o "axon_board.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(GEN_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "axon_routine.o" "driver_config.o" "ti_msp_dl_config.o" "startup_mspm0g350x_ticlang.o" "isr.o" "main.o" "soma_uart_test.o" "uart_packet.o" "driver\UART.o" "driver\dma\DMAMSPM0.o" "driver\uart\UARTMSPM0.o" "driver\utils\List.o" "driver\utils\RingBuf.o" "driver\utils\StructRingBuf.o" "peripheral\fram_memory_map.o" "peripheral\msp_peripheral_config.o" "protcol\src\s2a_client.o" "protcol\src\t2s_packet.o" "protcol\src\target.o" 
	-$(RM) "axon_routine.d" "driver_config.d" "ti_msp_dl_config.d" "startup_mspm0g350x_ticlang.d" "isr.d" "main.d" "soma_uart_test.d" "startup_mspm0g350x_ticlang.d" "ti_msp_dl_config.d" "uart_packet.d" "driver\UART.d" "driver\dma\DMAMSPM0.d" "driver\uart\UARTMSPM0.d" "driver\utils\List.d" "driver\utils\RingBuf.d" "driver\utils\StructRingBuf.d" "peripheral\fram_memory_map.d" "peripheral\msp_peripheral_config.d" "protcol\src\s2a_client.d" "protcol\src\t2s_packet.d" "protcol\src\target.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

