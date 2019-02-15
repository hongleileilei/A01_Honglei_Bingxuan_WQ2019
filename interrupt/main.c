
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
#include "string.h"

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
#include "spi.h"
#include "uart.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"
#include "timer_if.h"
#include "gpio_if.h"


#define SPI_IF_BIT_RATE  100000
#define BLACK           0x0000
#define WHITE           0xFFFF

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long count;
volatile unsigned long value;
volatile unsigned long lastValue;
volatile unsigned long diff;
volatile unsigned char nextFlag;
volatile unsigned char confirmFlag;
volatile unsigned char isNew;
static volatile unsigned long g_lastKey;
static volatile unsigned long g_ulBase;
char cmp[60];
char cmpCheck[60];
char ifRepeat[60];
char buf[60];
int i;
int j;
int k;
int x;
int y;
int Rx;
int Ry;
int m;
int maxLength;

char *Button0 = "1010000000000010000000001000000001001100010011001";
char *Button1 = "1010000000000010000000001000000000000100000001001";
char *Button2 = "1010000000000010000000001000000001000100010001001";
char *Button3 = "1010000000000010000000001000000000100100001001001";
char *Button4 = "1010000000000010000000001000000001100100011001001";
char *Button5 = "1010000000000010000000001000000000010100000101001";
char *Button6 = "1010000000000010000000001000000001010100010101001";
char *Button7 = "1010000000000010000000001000000000110100001101001";
char *Button8 = "1010000000000010000000001000000001110100011101001";
char *Button9 = "1010000000000010000000001000000000001100000011001";
char *LAST = "1010000000000010000000001000000001110110011101101";
char *MUTE = "1010000000000010000000001000000000100110001001101";
char checkList[12][4] = {
                     {' '}, //Button 0
                     {',', '.', '!'}, //Button 1
                     {'a', 'b', 'c'}, //Button 2
                     {'d', 'e', 'f'}, //Button 3
                     {'g', 'h', 'i'}, //Button 4
                     {'j', 'k', 'l'}, //Button 5
                     {'m', 'n', 'o'}, //Button 6
                     {'p', 'q', 'r', 's'}, //Button 7
                     {'t', 'u', 'v'}, //Button 8
                     {'w', 'x', 'y', 'z'}, //Button 9
                     {'+'}, //Button LAST
                     {'-'}, //Button MUTE

};
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static void BoardInit(void);

int time_diff(unsigned long value, unsigned long lastValue) {
    unsigned long interval;
    if (lastValue > value) interval = (lastValue - value)/80000; //80000
    else interval = (2^24 + (lastValue - value))/80000;

    return interval;
}
//*****************************************************************************
//
//check input character
//
//*****************************************************************************
static char
checkButton(char *cmp) {
    char message;
    if(strcmp(cmp, Button0) == 0) { message = '0';}
    else if(strcmp(cmp, Button1) == 0) { message = '1';}
    else if(strcmp(cmp, Button2) == 0) { message = '2';}
    else if(strcmp(cmp, Button3) == 0) { message = '3';}
    else if(strcmp(cmp, Button4) == 0) { message = '4';}
    else if(strcmp(cmp, Button5) == 0) { message = '5';}
    else if(strcmp(cmp, Button6) == 0) { message = '6';}
    else if(strcmp(cmp, Button7) == 0) { message = '7';}
    else if(strcmp(cmp, Button8) == 0) { message = '8';}
    else if(strcmp(cmp, Button9) == 0) { message = '9';}
    else if(strcmp(cmp, LAST) == 0) { message = '+';}
    else if(strcmp(cmp, MUTE) == 0) { message = '-';}
    else{message = 'x';}

    return message;
    //return checkList[i][j];
}

//*****************************************************************************
//
//check pressed button
//
//*****************************************************************************
static int
checkRow(char text) {
    int message;
    if(text == '0') { message = 0; maxLength = 0;}
    else if(text == '1') { message = 1; maxLength = 2;}
    else if(text == '2') { message = 2; maxLength = 2;}
    else if(text == '3') { message = 3; maxLength = 2;}
    else if(text == '4') { message = 4; maxLength = 2;}
    else if(text == '5') { message = 5; maxLength = 2;}
    else if(text == '6') { message = 6; maxLength = 2;}
    else if(text == '7') { message = 7; maxLength = 3;}
    else if(text == '8') { message = 8; maxLength = 2;}
    else if(text == '9') { message = 9; maxLength = 3;}
    else if(text == '+') { message = 10; maxLength = 0;}
    else if(text == '-') { message = 11; maxLength = 0;}
    else{message = 12;}

    return message;
    //return checkList[i][j];
}

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************
static void GPIOA1IntHandler(void) { // ############################
    unsigned long ulStatus;
    unsigned long interval;
    char text;
    int l;


    lastValue = value;
    value = SysTickValueGet();
    diff = time_diff(value, lastValue);

    if(diff >= 10 ){
        k = 0;
        count = 0;
    }else{
        if(diff >= 1){
            cmp[k] = '1';
            k++;
        }
        else{
            cmp[k] = '0';
            k++;
        }
        count++;

        if(count >= 49){
            cmp[k] = '\0';
            text = checkButton(cmp);
            if (g_lastKey == -1) MAP_TimerEnable(TIMERA0_BASE, TIMER_A);
            unsigned long tik = Timer_IF_GetCount(TIMERA0_BASE, TIMER_A);
            unsigned long tok = g_lastKey;
            g_lastKey = tik;
            int wait = 535 - (tik - tok)/8000000;
            if(wait>2 && wait<20) {nextFlag = 0;}
            else if (wait >= 20) {
                nextFlag = 1;
                j = 0;
            }
            else {nextFlag = 2;}

            if(strcmp(cmp, cmpCheck) != 0 || text == 'x' || text == '+' || text == '-' || nextFlag == 1){
                //Report("------------ %d\r\n", interval);
                if(text != 'x' && text != '+' && text != '-' && confirmFlag == 1){
                    m++;
                    x+=8;
                    if(x > 127){
                        x = 0;
                        y+=8;
                        if(y > 127){
                            y = 0;
                        }
                    }
                }
                if(text == '-'){
                    buf[m] = '\0';
                    m--;
                    drawChar(x, y, ' ', WHITE, BLACK, 1);
                    x -= 8;
                    if (x == 0 && y >= 8) {
                      x = 119;
                      y -= 8;
                    }
                    if (x <= 3 && y <= 3) {
                      x = 0;
                      y = 0;
                      confirmFlag = 0;
                    }
                }
                else if(text == '+'){
                    fillRect(0, 0, 127, 63, BLACK);
                    buf[m+1] = '\0';
                    int n;
                    for(n = 0; n <= strlen(buf); n++) {MAP_UARTCharPut(UARTA1_BASE, buf[n]);}
                    //MAP_UARTCharPut(UARTA1_BASE, '\0');
                    x = 0;
                    y = 0;
                    m = 0;
                    confirmFlag = 0;
                }
                i = checkRow(text);
                j = 0;
                if(i != 12 && i != 11 && i != 10) {
                    drawChar(x, y, checkList[i][j], WHITE, BLACK, 1);
                    buf[m] = checkList[i][j];
                }
                if(confirmFlag == 0 && i != 11 && i != 10) {confirmFlag = 1;}
                if(text == 'x') {Report("This key is not in the target button set.\r\n", text);}
                else if(text == '+') {Report("Button ENTER/LAST is pressed.\r\n");}
                else if(text == '-') {Report("Button DELETE/MUTE is pressed.\r\n");}
                else {Report("Button #%c is pressed.\r\n", text);}
                strcpy(cmpCheck, cmp);
            }
            else if(strcmp(cmp, cmpCheck) == 0 && nextFlag == 0 && j < maxLength){
                j++;
                if(i != 12 && i != 11 && i != 10) {
                    drawChar(x, y, checkList[i][j], WHITE, BLACK, 1);
                    buf[m] = checkList[i][j];
                }
            }
        }
    }

    ulStatus = MAP_GPIOIntStatus(GPIOA1_BASE, true);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);        // clear interrupts on GPIOA1
}

static void UARTIntHandler(void) {

    unsigned char transChar;
    MAP_UARTIntDisable(UARTA1_BASE,UART_INT_RX);

    while(UARTCharsAvail(UARTA1_BASE))
    {
        transChar = MAP_UARTCharGet(UARTA1_BASE);
        if(isNew)
        {
            fillRect(0, 65, 127, 127, BLACK);
            isNew = 0;
        }

        if(Rx > 127) // About to go off right edge.
        {
            Rx = 0;
            Ry += 8; // go to next line.
        }
        if(transChar == '\0')
        {
            isNew = 1;
            Rx = 0;
            Ry = 65;
        }
        else
        {
            drawChar(Rx, Ry, transChar, WHITE, BLACK, 1);
            Rx += 8;
        }
    }
    MAP_UARTIntClear(UARTA1_BASE,UART_INT_RX);
    MAP_UARTIntEnable(UARTA1_BASE,UART_INT_RX);
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
    // Initialize the device on UART1
    //
    MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE,UART_INT_RX);
    MAP_UARTFIFOLevelSet(UARTA1_BASE,UART_FIFO_TX1_8,UART_FIFO_RX1_8);

    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                     UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));

    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(GPIOA1_BASE, GPIOA1IntHandler);

    //
    // Configure rising edge interrupts on GPIO
    //
    MAP_GPIOIntTypeSet(GPIOA1_BASE, 0x40, GPIO_RISING_EDGE);    // GPIO - output, GPIO_RISING_EDGE or GPIO_BOTH_EDGES


    ulStatus = MAP_GPIOIntStatus (GPIOA1_BASE, false);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);    // clear interrupts on GPIOA1

    SysTickPeriodSet(16777216);
    //Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_ONE_SHOT_UP, TIMER_A, 0);//ulConfig check
    SysTickEnable();

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_ONE_SHOT_UP, TIMER_A, 0); //ulConfig check

    // Enable GPIO interrupts


    // clear global variables
    i = 0;
    j = 0;
    k = 0;
    x = 0;
    y = 0;
    Rx = 0;
    Ry = 65;
    m = 0;
    maxLength = 0;
    count = 0;
    value = 0;
    lastValue = 0;
    nextFlag = 0;
    confirmFlag = 0;
    isNew = 1;
    memset(cmpCheck, '\0', sizeof(cmpCheck));
    strcpy(cmpCheck, "00000000000000000000000000000000");
    memset(ifRepeat, '\0', sizeof(cmpCheck));
    strcpy(ifRepeat, "00000000000000000000000000000000");

    // Enable GPIO interrupts
    MAP_GPIOIntEnable(GPIOA1_BASE, 0x40);
    MAP_TimerEnable(TIMERA0_BASE, TIMER_A);

    Message("\t\t****************************************************\n\r");
    Message("\t\t\t IR Remote TX/RX Application Start \n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");
    Report("count = %d\r\n",count);

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                    SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                    (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVEHIGH |
                    SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    //
    // Initialising the OLED.
    //
    Adafruit_Init();
    fillScreen(BLACK);
    drawLine(0, 64, 127, 64, WHITE);
    while (1) {
      if(x > 127 || y > 127 || Rx > 127 || Ry > 127){
            break;
        }
    }

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
