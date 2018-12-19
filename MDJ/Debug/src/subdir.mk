################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola_MDJ.c \
../src/MDJ.c 

OBJS += \
./src/Consola_MDJ.o \
./src/MDJ.o 

C_DEPS += \
./src/Consola_MDJ.d \
./src/MDJ.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-2c-RogerThat/FuncionesConexiones" -I"/home/utnso/workspace/tp-2018-2c-RogerThat/MDJ/lib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


