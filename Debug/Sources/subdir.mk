################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/FIFO.c \
../Sources/Flash.c \
../Sources/Frequnency.c \
../Sources/LEDs.c \
../Sources/PIT.c \
../Sources/RMS.c \
../Sources/Spectrum.c \
../Sources/UART.c \
../Sources/VRR.c \
../Sources/handle.c \
../Sources/main.c \
../Sources/packet.c 

OBJS += \
./Sources/FIFO.o \
./Sources/Flash.o \
./Sources/Frequnency.o \
./Sources/LEDs.o \
./Sources/PIT.o \
./Sources/RMS.o \
./Sources/Spectrum.o \
./Sources/UART.o \
./Sources/VRR.o \
./Sources/handle.o \
./Sources/main.o \
./Sources/packet.o 

C_DEPS += \
./Sources/FIFO.d \
./Sources/Flash.d \
./Sources/Frequnency.d \
./Sources/LEDs.d \
./Sources/PIT.d \
./Sources/RMS.d \
./Sources/Spectrum.d \
./Sources/UART.d \
./Sources/VRR.d \
./Sources/handle.d \
./Sources/main.d \
./Sources/packet.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"C:\Users\theod\workspace.kds\Project\Library" -I"C:/Users/theod/workspace.kds/Project/Static_Code/IO_Map" -I"C:/Users/theod/workspace.kds/Project/Sources" -I"C:/Users/theod/workspace.kds/Project/Generated_Code" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


