
//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//                          GPIO interrupts using SW2 and SW3.
//                          NOTE: the switches are not debounced!
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup int_sw
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "systick.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"
#include "timer_if.h"
#include "gpio_if.h"

#define ti2ms(ts) (((double) 1000 / (double) 80000000) * (double)(ts))
#define Button0 (int)0b1010000000000010000000001000000001001100010011001
#define Button1 (int)0b1010000000000010000000001000000000000100000001001
#define Button2 (int)0b1010000000000010000000001000000001000100010001001
#define Button3 (int)0b1010000000000010000000001000000000100100001001001
#define Button4 (int)0b1010000000000010000000001000000001100100011001001
#define Button5 (int)0b1010000000000010000000001000000000010100000101001
#define Button6 (int)0b1010000000000010000000001000000001010100010101001
#define Button7 (int)0b1010000000000010000000001000000000110100001101001
#define Button8 (int)0b1010000000000010000000001000000001110100011101001
#define Button9 (int)0b1010000000000010000000001000000000001100000011001
#define LAST (int)0b1010000000000010000000001000000001110110011101101
#define MUTE (int)0b1010000000000010000000001000000000100110001001101

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile int cmp;
volatile int i;
volatile int j;
volatile unsigned long count;
volatile unsigned long value;
volatile unsigned long lastValue;
volatile unsigned long diff;
volatile unsigned char confirmFlag;
volatile unsigned char passFlag;
volatile unsigned char readFlag;
volatile unsigned char startTimer;
static volatile unsigned long g_ulBase;
volatile char checkList[12][4] = {
                     {'_'}, //Button 0
                     {',', '.', '!'}, //Button 1
                     {'a', 'b', 'c'}, //Button 2
                     {'d', 'e', 'f'}, //Button 3
                     {'g', 'h', 'i'}, //Button 4
                     {'j', 'k', 'l'}, //Button 5
                     {'m', 'n', 'o'}, //Button 6
                     {'p', 'q', 'r', 's'}, //Button 7
                     {'t', 'u', 'v'}, //Button 8
                     {'w', 'x', 'y', 'z'}, //Button 9
                     {'DELETE'}, //Button LAST
                     {'ENTER'}, //Button MUTE

};

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************
static void GPIOA1IntHandler(void) { // ############################
    unsigned long ulStatus;

    lastValue = value;
    value = SysTickValueGet();
    diff = (lastValue - value)/80000;

// passFlag might be used here for long press
    if(diff > 10){
        count = 0;
        readFlag = 1;
        Report("Ready to read the new input ...\r\n");
        SysTickPeriodSet(16777216);
        SysTickEnable();
    }
    else{
        if(count == 0){ // double check
            diff = 1;
            cmp += diff;
            cmp <<= 2;
        }
        else if(diff > 1){
            diff = 1; //can be deleted
            cmp += diff;
            cmp <<= 1;
        }
        else{
            diff = 0; //can be deleted
            cmp <<= 1;
        }
        Report("#%d -> VALUE = %d\r\n", count, diff);
        count++;
    }

    ulStatus = MAP_GPIOIntStatus(GPIOA1_BASE, true);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);        // clear interrupts on GPIOA1
}

//*****************************************************************************
//
//check input character
//
//*****************************************************************************
static char
checkChar(void) {
        switch(cmp){
            case Button0:
                if(i != 0) {confirmFlag = 1;}
                break;
            case Button1:
                if(i != 1) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button2:
                if(i != 2) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button3:
                if(i != 3) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button4:
                if(i != 4) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button5:
                if(i != 5) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button6:
                if(i != 6) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button7:
                if(i != 7) {confirmFlag = 1;}
                else if(i = 0 && j < 4) {j++;}
                break;
            case Button8:
                if(i != 8) {confirmFlag = 1;}
                else if(i = 0 && j < 3) {j++;}
                break;
            case Button9:
                if(i != 9) {confirmFlag = 1;}
                else if(i = 0 && j < 4) {j++;}
                break;
            case LAST:
                if(i != 10) {confirmFlag = 1;}
                break;
            case MUTE:
                if(i != 11) {confirmFlag = 1;}
                break;
            default:
                Report("Not in the target button set.\r\n");
        }
        cmp = 0b0;

        return checkList[i][j];
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);

    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
//****************************************************************************
//
//! Main function
//!
//! \param none
//!
//!
//! \return None.
//
//****************************************************************************

int main() {
    unsigned long ulStatus;
    g_ulBase = TIMERA0_BASE;

    BoardInit();

    PinMuxConfig();

    InitTerm();

    ClearTerm();

    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(GPIOA1_BASE, GPIOA1IntHandler); // ############################

    //
    // Configure rising edge interrupts on GPIO
    //
    MAP_GPIOIntTypeSet(GPIOA1_BASE, 0x40, GPIO_RISING_EDGE);    // GPIO - output, GPIO_RISING_EDGE or GPIO_BOTH_EDGES


    ulStatus = MAP_GPIOIntStatus (GPIOA1_BASE, false);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);            // clear interrupts on GPIOA1



    SysTickPeriodSet(16777216);
    //Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_ONE_SHOT_UP, TIMER_A, 0);//ulConfig check
    SysTickEnable();




    // clear global variables
    cmp = 0b0;
    i = 0;
    j = 0;
    count = 0;
    value = 0;
    passFlag = 0;
    confirmFlag = 0;
    readFlag = 0;
    startTimer = 1;

    // Enable GPIO interrupts
    MAP_GPIOIntEnable(GPIOA1_BASE, 0x40);


    Message("\t\t****************************************************\n\r");
    Message("\t\t\t GPIO Interrupt Successfully Set Up \n\r");
    Message("\t\t ****************************************************\n\r");



    Message("\t\t****************************************************\n\r");
    Message("\t\t\t IR Remote TX/RX Application Start \n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");
    Report("count = %d\r\n",count);

    while (1) {


        //MAP_UtilsDelay(80000000/5);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
