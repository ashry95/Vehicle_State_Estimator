
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
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
uint32_t DistanceInmPs;
uint32_t speed;
uint32_t g_ui32Flags;
uint32_t StopFlag=0;
volatile uint8_t IncomingTime[2]={0};
volatile uint8_t IncomingSpeed=0;
volatile uint8_t StartTime[2]={0};
volatile uint8_t startSpeed=0;
volatile uint8_t UART1Counter=0;

void Calc_Dist(void);

void Timer0IntHandler(void)
{
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    HWREGBITW(&g_ui32Flags, 0) ^= 1;
    ROM_IntMasterDisable();
    ROM_IntMasterEnable();
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
        if(UART0_Incoming == 'o')
        {
        }
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
        UART1Counter++;
        switch (UART1Counter)
        {
        case 1:
            IncomingTime[0] = UART1_Incoming;
            break;
        case 2:
            IncomingTime[1] = UART1_Incoming;
            break;
        case 3 :
            IncomingSpeed = UART1_Incoming;
            Calc_Dist();
            UART1Counter=0;
            break;
        default :

            break;
        }

    }
}

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
    uint32_t pinval1 = 0;
    uint32_t pinval2 = 0;
    while(1)
    {
        pinval1=GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
        if(pinval1==0)
        {
            UARTCharPut(UART1_BASE,'s');
            DistanceInmPs = 0;
            StopFlag = 0;
            while(!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4));
            SysCtlDelay(10000);
            StartTime[0]=IncomingTime[0];
            StartTime[1]=IncomingTime[1];
            startSpeed=IncomingSpeed;
        }
        else
        {
            /*Do nothing*/
        }
        pinval2=GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);

        if(pinval2==0)
        {
            while(!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4));
            SysCtlDelay(10000);
            StopFlag=1;
        }
        else
        {
            /*Do nothing*/
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

void UART_Init0(void)
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}

void Timers_Init(void)
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet());
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}
void Calc_Dist(void)
{
    if(!StopFlag){
        DistanceInmPs+=IncomingSpeed;
    }
}

