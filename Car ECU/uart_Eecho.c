//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "inc/hw_uart.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/interrupt.h"
#include"driverlib/timer.h"



void CalculateTime(void);
void CalculateDistance(void);

#define SECONDS_PER_MINUATE  (59)
#define MINUATES_PER_HOUR    (59)
#define HOURS_PER_DAY        (24)


typedef enum
{
    WAIT,
    INCREMENT,
    STOP
}enFlag_t;

static volatile enFlag_t TimeFlag = WAIT;
static volatile enFlag_t DistanceFlag = WAIT;


static uint8_t Seconds = 0;
static uint32_t CurrentDistance = 0;
static uint8_t CurrentSpeed= 0;

struct strTime
{
    uint8_t Hours;
    uint8_t Minuates;
};

struct strTime CurrentTime;
struct strTime TimeSample;

uint32_t g_ui32Flags;
volatile uint8_t g_flags=0;
void Timer0IntHandler(void)
{

    char cOne, cTwo;
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    HWREGBITW(&g_ui32Flags, 0) ^= 1;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, g_ui32Flags << 1);
    ROM_IntMasterDisable();
    cOne = HWREGBITW(&g_ui32Flags, 0) ? '1' : '0';
    cTwo = HWREGBITW(&g_ui32Flags, 1) ? '1' : '0';
    ROM_IntMasterEnable();


    TimeFlag = INCREMENT;
}


void CalculateTime(void)
{
    if(TimeFlag)
    {
        Seconds++;
        if(Seconds == SECONDS_PER_MINUATE )
        {
            Seconds = 0;
            CurrentTime.Minuates++;
            if(g_flags==1)
            {
                UARTCharPut(UART1_BASE, CurrentTime.Hours);
                UARTCharPut(UART1_BASE, CurrentTime.Minuates);
                UARTCharPut(UART1_BASE, CurrentSpeed);
            }
        }
        if(CurrentTime.Minuates == MINUATES_PER_HOUR)
        {
            CurrentTime.Minuates = 0;
            CurrentTime.Hours++;

        }
        if(CurrentTime.Hours == HOURS_PER_DAY)
        {
            CurrentTime.Hours = 0;
        }

        TimeFlag = WAIT;

    }
}



void CalculateDistance(void)
{
    if(DistanceFlag == INCREMENT)
    {
        CurrentDistance += CurrentSpeed;
        DistanceFlag = WAIT;
    }
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************/
volatile uint32_t UART0_Incoming=0;
volatile uint32_t UART1_Incoming=0;

void UARTIntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    ROM_UARTIntClear(UART0_BASE, ui32Status);

    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        UART0_Incoming=ROM_UARTCharGetNonBlocking(UART0_BASE);
    }
}

void UARTIntHandler_1(void)
{

    uint32_t ui32Status2;

    ui32Status2 = ROM_UARTIntStatus(UART1_BASE, true);

    ROM_UARTIntClear(UART1_BASE, ui32Status2);

    while(ROM_UARTCharsAvail(UART1_BASE))
    {
        UART1_Incoming=ROM_UARTCharGetNonBlocking(UART1_BASE);
        if(UART1_Incoming=='s')
        {
            g_flags=1;
            UARTCharPut(UART1_BASE, CurrentTime.Hours);
            UARTCharPut(UART1_BASE, CurrentTime.Minuates);
            UARTCharPut(UART1_BASE, CurrentSpeed);
        }
    }


    DistanceFlag = INCREMENT;
}


volatile uint32_t pinval1;
volatile uint32_t pinval2;

void UART_Init0(void);
void UART_Init1(void);
void Timers_Init(void);


int main(void)
{

    ROM_FPULazyStackingEnable();

    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    Timers_Init();
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    UART_Init0();
    UART_Init1();
    ROM_IntMasterEnable();
    ROM_IntEnable(INT_UART0);
    ROM_IntEnable(INT_UART1);

    CurrentTime.Minuates = (UARTCharGet(UART0_BASE) - '0');
    CurrentTime.Minuates += ((UARTCharGet(UART0_BASE) - '0') * 10);
    CurrentTime.Hours = (UARTCharGet(UART0_BASE) - '0');
    CurrentTime.Hours += ((UARTCharGet(UART0_BASE) - '0') * 10);
    Seconds = 0;

    uint8_t l_Previous_speed=0;
    while(1)
    {
        pinval1=GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
        if(pinval1==0)
        {
            CurrentSpeed--;
            while(!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4));
            // SysCtlDelay(10000);
        }

        pinval2=GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);

        if(pinval2==0)
        {
            CurrentSpeed++;
            while(!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0));
            //(10000);
        }
        CalculateTime();
        if( l_Previous_speed != CurrentSpeed )
        {
                UARTCharPut(UART1_BASE, CurrentTime.Hours);
                UARTCharPut(UART1_BASE, CurrentTime.Minuates);
                UARTCharPut(UART1_BASE, CurrentSpeed);
            l_Previous_speed = CurrentSpeed;
        }
    }

}


void UART_Init1(void){

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART1_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                    UART_CONFIG_PAR_NONE));

    ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
}


void UART_Init0(void){
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}


void Timers_Init(void){
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet());
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}
