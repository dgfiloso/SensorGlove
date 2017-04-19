/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== sensor_glove.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
#include <ti/sysbios/family/arm/m3/Power.h>
#include <driverlib/rom.h>

/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>
#include <ti/drivers/ADC.h>
#include <ti/drivers/adc/ADCCC26XX.h>
#include <driverlib/aux_adc.h>
#include <driverlib/aux_wuc.h>
#include <inc/hw_aux_evctl.h>

#include "SensorMpu9250.h"
#include "SensorI2C.h"

/* Board Header files */
#include "Board.h"

#define TASK0STACKSIZE   512
#define TASK1STACKSIZE   1024
#define TASK2STACKSIZE   512

#define SAMPLECOUNT 8
#define SAMPLETYPE uint16_t
#define SAMPLESIZE sizeof(SAMPLETYPE)

/*  Task Structs and variables  */
static Task_Struct task0Struct;
static Task_Struct task1Struct;
static Task_Struct task2Struct;
static Char task0Stack[TASK0STACKSIZE];
static Char task1Stack[TASK1STACKSIZE];
static Char task2Stack[TASK2STACKSIZE];

/*  Semaphore Structs and variables */
static Semaphore_Struct sem;
static Semaphore_Handle hSem;

/*  ADC Structs and variables   */
static Hwi_Struct hwi;
static SAMPLETYPE adcSamples[SAMPLECOUNT];
static SAMPLETYPE singleSample;

/* Pin driver handle */
static PIN_Handle pinHandle;
static PIN_State pinState;

PIN_Config pinTable[] = {
    Board_LED0      | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1      | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_DP2       | PIN_INPUT_EN       | PIN_BM_HYSTERESIS           | PIN_BM_PULLING ,
    Board_I2C0_SDA1 | PIN_INPUT_EN       | PIN_PULLUP   | PIN_OPENDRAIN,
    Board_I2C0_SCL1 | PIN_INPUT_EN       | PIN_PULLUP   | PIN_OPENDRAIN,
    PIN_TERMINATE
};

/*  Tasks   ----------------------------------------------------------------------------*/

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
Void heartBeatFxn(UArg arg0, UArg arg1)
{
    while (1) {
        Task_sleep((UInt)arg0);
        PIN_setOutputValue(pinHandle, Board_LED0,
                           !PIN_getOutputValue(Board_LED0));
    }
}

Void taskMpu9250 (UArg arg0, UArg arg1)
{
    uint16_t    accelRawData[3];
    uint16_t    gyroRawData[3];
    float       accelConvData[3];
    float       gyroConvData[3];
    int         i;

    while (!SensorI2C_open())
    {
        System_printf("Cannot open I2C\n");
        System_flush();
    }

    SensorMpu9250_powerOn();
    while (!SensorMpu9250_init())
    {
        System_printf("Cannot initialize MPU9250\n");
        System_flush();
    }

    SensorMpu9250_enable(6);        //  Enable all axes
    while (!SensorMpu9250_accSetRange(ACC_RANGE_16G))
    {
        System_printf("Accelerometer range set failed\n");
        System_flush();
    }

    while(1)
    {
        while (!SensorMpu9250_accRead(accelRawData))
        {
            System_printf("Imposible to read accelerometer data\n");
            System_flush();
        }
        while (!SensorMpu9250_gyroRead(gyroRawData))
        {
            System_printf("Imposible to read gyro data\n");
            System_flush();
        }

        for(i=0; i<3; i++)
        {
            accelConvData[i] = SensorMpu9250_accConvert(accelRawData[i]);
            gyroConvData[i] = SensorMpu9250_gyroConvert(gyroRawData[i]);
        }
        System_printf("Accel: %d, %d, %d \t Gyro: %d, %d, %d \n", accelConvData[0], accelConvData[1], accelConvData[2], gyroConvData[0], gyroConvData[1], gyroConvData[2]);
        System_flush();
    }
}

//Void taskFlexSensor (UArg arg0, UArg arg1)
//{
//    int32_t adcVoltage;
//
//    // Enable clock for ADC digital and analog interface (not currently enabled in driver)
//    AUXWUCClockEnable(AUX_WUC_ADI_CLOCK | AUX_WUC_ADC_CLOCK | AUX_WUC_OSCCTRL_CLOCK);
//    // Connect AUX IO7 (DIO23) as analog input.
//    AUXADCSelectInput(ADC_COMPB_IN_AUXIO7);
//
//    // Set up ADC
//    AUXADCEnableSync(AUXADC_REF_FIXED, AUXADC_SAMPLE_TIME_2P7_US, AUXADC_TRIGGER_MANUAL);
//
//    while(1)
//    {
//        // Trigger ADC sampling
//        AUXADCGenManualTrigger();
//
//        Task_sleep(100 * 1000 / Clock_tickPeriod);
//
//        singleSample = AUXADCReadFifo();
//        adcVoltage = AUXADCValueToMicrovolts(AUXADC_FIXED_REF_VOLTAGE_NORMAL, singleSample);
//
//        System_printf("Muestra: %d mV", adcVoltage);
//        System_flush();
//
//    }


/************************************************************************************/
//    ADC_Handle adc;
//    ADC_Params params;
//    int_fast16_t res;
//    uint_fast16_t adcValue;
//    uint32_t adcConv;
//
//    ADC_Params_init(&params);
//    adc = ADC_open(Board_ADC0, &params);
//
//    while(adc != NULL)
//    {
//        res = ADC_convert(adc, &adcValue);
//        if (res == ADC_STATUS_SUCCESS)
//        {
//            adcConv = ADC_convertRawToMicroVolts(adc, adcValue);
//            System_printf("ADC = %d", adcConv);
//        }
//    }
//
//    ADC_close(adc);
//}
/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params task0Params;
    Task_Params task1Params;
//    Task_Params task2Params;

    /* Call board init functions */
    Board_initGeneral();
//    Board_initI2C();
    // Board_initSPI();
    // Board_initUART();
    // Board_initWatchdog();

    /* Construct heartBeat Task  thread */
    Task_Params_init(&task0Params);
    task0Params.arg0 = 1000000 / Clock_tickPeriod;
    task0Params.stackSize = TASK0STACKSIZE;
    task0Params.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)heartBeatFxn, &task0Params, NULL);

    /* Construct MPU9250 Task  thread */
    Task_Params_init(&task1Params);
    task1Params.stackSize = TASK1STACKSIZE;
    task1Params.stack = &task1Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)taskMpu9250, &task1Params, NULL);

//    /*  Construct FlexSensor Task thread */
//    Task_Params_init(&task2Params);
//    task2Params.stackSize = TASK2STACKSIZE;
//    task2Params.stack = &task2Stack;
//    Task_construct(&task2Struct, (Task_FuncPtr)taskFlexSensor, &task2Params, NULL);

    // Construct semaphore used for pending in task
    Semaphore_Params sParams;
    Semaphore_Params_init(&sParams);
    sParams.mode = Semaphore_Mode_BINARY;

    Semaphore_construct(&sem, 0, &sParams);
    hSem = Semaphore_handle(&sem);


    /* Open LED pins */
    pinHandle = PIN_open(&pinState, pinTable);
    if(!pinHandle) {
        System_abort("Error initializing board pins\n");
    }

    PIN_setOutputValue(pinHandle, Board_LED1, 1);

    System_printf("Starting the example\nSystem provider is set to SysMin. "
                  "Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}