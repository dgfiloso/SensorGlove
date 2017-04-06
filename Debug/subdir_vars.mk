################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../empty.cfg 

CMD_SRCS += \
../CC2650STK.cmd 

C_SRCS += \
../CC2650STK.c \
../SensorI2C.c \
../SensorMpu9250.c \
../SensorOpt3001.c \
../SensorUtil.c \
../ccfg.c \
../sensor_glove.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./CC2650STK.d \
./SensorI2C.d \
./SensorMpu9250.d \
./SensorOpt3001.d \
./SensorUtil.d \
./ccfg.d \
./sensor_glove.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./CC2650STK.obj \
./SensorI2C.obj \
./SensorMpu9250.obj \
./SensorOpt3001.obj \
./SensorUtil.obj \
./ccfg.obj \
./sensor_glove.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

OBJS__QUOTED += \
"CC2650STK.obj" \
"SensorI2C.obj" \
"SensorMpu9250.obj" \
"SensorOpt3001.obj" \
"SensorUtil.obj" \
"ccfg.obj" \
"sensor_glove.obj" 

C_DEPS__QUOTED += \
"CC2650STK.d" \
"SensorI2C.d" \
"SensorMpu9250.d" \
"SensorOpt3001.d" \
"SensorUtil.d" \
"ccfg.d" \
"sensor_glove.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

C_SRCS__QUOTED += \
"../CC2650STK.c" \
"../SensorI2C.c" \
"../SensorMpu9250.c" \
"../SensorOpt3001.c" \
"../SensorUtil.c" \
"../ccfg.c" \
"../sensor_glove.c" 


