################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola_S-AFA.c \
../src/S-AFA.c 

OBJS += \
./src/Consola_S-AFA.o \
./src/S-AFA.o 

C_DEPS += \
./src/Consola_S-AFA.d \
./src/S-AFA.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-2c-RogerThat/FuncionesConexiones" -I"/home/utnso/workspace/tp-2018-2c-RogerThat/S-AFA/lib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


