/************************************************************************
*                                                                       *
*   Filename:       setup.h                                             *
*   Date:           02/06/2015                                          *
*   File Version:   1.0                                                 *
*                                                                       *
*   Author:         Andrew Cullen                                       *
*                                                                       *
*************************************************************************
*                                                                       *
*   Architecture:   Mid-range PIC                                       *
*   Processor:      PIC16F917                                           *
*   Compiler:       MPLAB XC8 v1.00 (Free mode)                         *
*                                                                       *
*************************************************************************
*                                                                       *
*   Files required: none                                                *
*                                                                       *
*************************************************************************
*                                                                       *
*   Description:    This file contains declarations, definitions and    *
*                   seting up registers for the WEBots Project Car      *
*                   battery management program                          *
*                                                                       *
*************************************************************************
*                                                                       *
*   Pin assignments:                                                    *
*                   C1_MEAS_UF      - AN0                               *
*                   C2_MEAS_UF      - AN1                               *
*                   1.982V_REF_UF   - AN2                               *
*                   C5_MEAS_UF      - AN3                               *
*                   TEMP_TH         - RA4                               *
*                   C6_MEAS_UF      - AN4                               *
*                   C3_MEAS_UF      - AN5                               *
*                   C4_MEAS_UF      - AN6                               *
*                   CURRENT_MEAS    - AN7                               *
*                   CG_SEL1         - RA7                               *
*                   CG_SEL0         - RA6                               *
*                   MOTOR_CONTROL   - RC0                               *
*                   R_LED           - RC1                               *
*                   OPAMP_CGND      - RC2                               *
*                   TP_CLS_EN       - RC3                               *
*                   BT_CLS_EN       - RD0                               *
*                   REF_EN          - RD1                               *
*                   LCD_DB7         - RB5                               *
*                   LCD_DB6         - RB4                               *
*                   LCD_DB5         - RB3                               *
*                   LCD_DB4         - RB2                               *
*                   LCD_EN          - RB1                               *
*                   LCD_RS          - RB0                               *
*                   LCD_RW          - RD7                               *
*                   LCD_PWR_SW      - RD6                               *
*                   LCD_PB          - RD5                               *
*                   G_LED           - RD4                               *
*                   E_STOP          - RC5                               *
*                   5V_REG2_EN      - RC4                               *
*                   12V_REG_EN      - RD3                               *
*                   UC_CGND         - RD2                               *
*                                                                       *
************************************************************************/

#ifndef SETUP_H
#define	SETUP_H

/***** CONFIGURATION *****/
/*
WDTE =	Watchdog Timer Enable bit
    ON      WDT enabled
    OFF     WDT disabled and can be enabled by SWDTEN bit of the WDTCON register
PWRTE =	Power Up Timer Enable bit
    OFF     PWRT disabled
    ON      PWRT enabled
CP =	Code Protection bit
    OFF     Program memory code protection is disabled
    ON      Program memory code protection is enabled
BOREN =	Brown-out Reset Selection bits
    ON      BOR enabled
    OFF     BOR disabled
    NSLEEP  BOR enabled during operation and disabled in Sleep
    SBODEN  BOR controlled by SBOREN bit of the PCON register
DEBUG =	In-Circuit Debugger Mode bit
    OFF     In-Circuit Debugger disabled, RB6/ISCPCLK and RB7/ICSPDAT are general purpose I/O pins
    ON      In-Circuit Debugger enabled, RB6/ICSPCLK and RB7/ICSPDAT are dedicated to the debugger
FCMEN =	Fail-Safe Clock Monitor Enabled bit
    ON      Fail-Safe Clock Monitor is enabled
    OFF     Fail-Safe Clock Monitor is disabled
MCLRE =	RE3/MCLR pin function select bit
    ON      RE3/MCLR pin function is MCLR
    OFF     RE3/MCLR pin function is digital input, MCLR internally tied to VDD
CPD =	Data Code Protection bit
    OFF     Data memory code protection is disabled
    ON      Data memory code protection is enabled
IESO =	Internal External Switchover bit
    ON      Internal/External Switchover mode is enabled
    OFF     Internal/External Switchover mode is disabled
FOSC =	Oscillator Selection bits
    HS          HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT/T1OSO and RA7/OSC1/CLKIN/T1OSI
    INTOSCIO	INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT/T1OSO pin, I/O function on RA7/OSC1/CLKIN/T1OSI
    INTOSCCLK	INTOSC oscillator: CLKOUT function on RA6/OSC2/CLKOUT/T1OSO pin, I/O function on RA7/OSC1/CLKIN/T1OSI
    LP          LP oscillator: Low-power crystal on RA6/OSC2/CLKOUT/T1OSO and RA7/OSC1/CLKIN/T1OSI
    EXTRCIO	RCIO oscillator: I/O function on RA6/OSC2/CLKOUT/T1OSO pin, RC on RA7/OSC1/CLKIN/T1OSI
    EC          EC: I/O function on RA6/OSC2/CLKOUT/T1OSO pin, CLKIN on RA7/OSC1/CLKIN/T1OSI
    XT          XT oscillator: Crystal/resonator on RA6/OSC2/CLKOUT/T1OSO and RA7/OSC1/CLKIN/T1OSI
    EXTRCCLK	RC oscillator: CLKOUT function on RA6/OSC2/CLKOUT/T1OSO pin, RC on RA7/OSC1/CLKIN/T1OSI
 */

#pragma config WDTE = OFF, PWRTE = OFF, CP = OFF, BOREN = ON, DEBUG = ON, FCMEN = OFF, MCLRE = ON, CPD = OFF, IESO = ON, FOSC = INTOSCIO

#include <xc.h>
#include <stdint.h> // allows access to more intuiative interger classes such as uint8_t, int16_t

//#define CIRCUIT_DEBUG
//#define SHUTOFF_DEBUG
#define CELL2CELLVOLT
//#define SAVE_MEMORY
//#define CURRENT_DEBUG

// I2C
// 2 byte unsigned int for voltage
// 1 byte unsigned int for current
// 1 byte of the three previous bytes XORed together

/***** LCD Definitions *****/

#define D7 RB5
#define D6 RB4
#define D5 RB3
#define D4 RB2
#define EN RB1
#define RS RB0
#define RW RD7
#define LCD_PWR_SW RD6
#define LCD_PB RD5

#define _XTAL_FREQ 8000000  // used for _delay function, specifies a clock frequency of 4MHz

#include "lcd.h" // header file to interface with 16x2 LCD screen
#include "timer0.h"
#include "adc.h"

/***** Cell Measurement Definitions *****/

#define REF AN2
#define C1_MEAS AN0
#define C2_MEAS AN1
#define C3_MEAS AN5
#define C4_MEAS AN6
#define C5_MEAS AN3
#define C6_MEAS AN4
#define REF_EN RD1
#define TP_CLS_EN RC3
#define BT_CLS_EN RD0
#define OPAMP_CGND RC2 

/***** Current Measurement Definitions *****/

#define CURRENT_MEAS AN7
#define CG_SEL1 RA7
#define CG_SEL0 RA6

/***** Shutoff/Enable Definitions *****/

#define UC_CGND RD2
#define MOTOR_CONTROL RC0
#define E_STOP RC5
#define REG2_5V_EN RC4
#define REG_12V_EN RD3
#define TEMP_TH RA4

/***** LED Definitions *****/

#define R_LED RC1
#define G_LED RD4

/***** Variables *****/

float cellVolt[6] = {0, 0, 0, 0, 0, 0}; // array to hold the voltages of each cell
float supVolt = 4.2; // supply voltage to be set by the sampleReference function

uint16_t refValue = 0; // ADC value of the reference voltage

float current = 0; // current flowing out of the battery
uint8_t currentGain[] = {200, 100, 50, 25}; // gain for the current sense module

uint8_t LCDDisplayMode = 0;

/***** Calibration Variables *****/

const float refVolt = 1.128; // calibration value for the reference voltage

const float cellVoltL = 3.2; // minimum cell voltage

const float cell1RR = 0.98751; // the resistor ratio of the cell 1 resistor divider, TopRes / BtmRes (OHM)

const float cell2RR = 2.11635; // the resistor ratio of the cell 2 resistor divider, TopRes / BtmRes (OHM)

const float cell3RR = 4.52138; // the resistor ratio of the cell 3 resistor divider, TopRes / BtmRes (OHM)

const float cell4RR = 6.63700; // the resistor ratio of the cell 4 resistor divider, TopRes / BtmRes (OHM)

const float cell5RR = 9.03161; // the resistor ratio of the cell 5 resistor divider, TopRes / BtmRes (OHM)

const float cell6RR = 11.1326; // the resistor ratio of the cell 6 resistor divider, TopRes / BtmRes (OHM)

const float shuntRes = 0.001269309; // the resistance of the current shunt in ohms
const uint8_t sampleNum = 300; // the number of ADC samples to be averaged
const float IIRnew = 0.2; // IIR filter constant for incoming data
const float IIRprev = 0.8; // IIR filter constant eor previous data

uint8_t sendData[4]; // array to hold the data to be sent through I2C
char countI2C = 4;
uint8_t BRCount = 0; // variable to count the number of brownouts

#include "i2c.h"

#ifndef SAVE_MEMORY
const char str1 [] = "Voltage:";
const char str2 [] = "Current:";
const char str3 [] = "Cell 1: ";
const char str4 [] = "Cell 2: ";
const char str5 [] = "Cell 3: ";
const char str6 [] = "Cell 4: ";
const char str7 [] = "Cell 5: ";
const char str8 [] = "Cell 6: ";
const char str9 [] = " V      ";
const char str10 [] = " A       ";
#endif



/***** Functions *****/

void initController ();
void interrupt isr();
void sampleReference();
void sampleBatteryCells ();
void displayLCD ( int );
void sampleCurrent ();
void currentGainInit ( uint8_t );
void handlePB ();
float batteryVoltage ();
float sampleVoltage(ADCChannel  chan);
uint8_t systemCheck ();
void initCellVolt();
void shutDown( uint8_t );



void initController ()
{
    // *** Oscillator Initiliazation
    OSCCONbits.IRCF = 0b111; // Select 8MHz as clock frequency
    OSCCONbits.SCS = 0; // Configuration word defines clock source

    PCONbits.SBOREN = 1; // enable the brownout restet control
    PCONbits.nBOR = 1; // reset brownout module

    // set to 1 for input, 0 for output
    TRISA = 0b00111111;
    TRISB = 0b00000000;
    TRISC = 0b00100000;
    TRISD = 0b01100000;
    TRISE = 0b00000111;

    ANSEL = 0xFF; // set all of the analog channels as analog inputs

    PORTA = 0;
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;
    PORTE = 0;
    
    CMCON0bits.CM = 0b111; // disable comparitors
    LCDCON = 0; // disable LCD Driver
    LVDCONbits.LVDEN = 0; // disable the low voltage detect
    CCP1CON = 0; // disable CCP modules
    CCP2CON = 0;
    OPTION_REGbits.nRBPU = 1; // weak pullups dissabled

    UC_CGND = 0; // connect the ground for the microcontroller (active low)

    initADC();
    
    timeSetup();

    initLCD();

    initCellVolt();

    //initI2C();

    // start reading in values to stabalize the running average values
    stopWatch(0);

    while (stopWatch(1) < 2000)
    {
        systemCheck();
    }

    
    // initial system check

    uint8_t tempCheck = systemCheck();

    if ( tempCheck != 0)
    {
        shutDown( tempCheck );
    }

    // initial check sucessful so connect power
    MOTOR_CONTROL = 1;
   
}

void interrupt isr()
{
    isrTimer0();

    //isrI2C();
}

// initializes the cell voltages, reduces the startup stabalization
void initCellVolt()
{
    // Turn on the unity feedback op amps
    OPAMP_CGND = 1;

    // sample each of the cell voltages
    BT_CLS_EN = 1; // turn on the first three voltage dividers

    __delay_ms(5); // wait for the voltage to level out

    // sample the bottom cells
    cellVolt[0] = ( cell1RR + 1 ) * sampleVoltage(CELL1); // cell 1
    cellVolt[1] = ( cell2RR + 1 ) * sampleVoltage(CELL2); // cell 2
    cellVolt[2] = ( cell3RR + 1 ) * sampleVoltage(CELL3); // cell 3

    BT_CLS_EN = 0; // turn off the first three voltage dividers
    TP_CLS_EN = 1; // turn on the last three voltage dividers

    __delay_ms(5);

    cellVolt[3] = ( cell4RR + 1 ) * sampleVoltage(CELL4); // cell 4
    cellVolt[4] = ( cell5RR + 1 ) * sampleVoltage(CELL5); // cell 5
    cellVolt[5] = ( cell6RR + 1 ) * sampleVoltage(CELL6); // cell 6

    TP_CLS_EN = 0; // turn off the last three voltage dividers

    OPAMP_CGND = 0; // turn off the voltage dividers
}
#endif	/* SETUP_H */