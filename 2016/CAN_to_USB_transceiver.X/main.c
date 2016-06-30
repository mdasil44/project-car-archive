#include "Init.h"
#include <uart.h>
//#include "CAN.h"

void main()
{
    /*
    CANInit();
    while (CANIsTransmitComplete());
    CANTransmit(0, 0, 0);
     */
    unsigned long i = 0;
    TRISDbits.TRISD2 = 0;
    UARTInit();
    while (1)
    {
        LATDbits.LATD2 = 1;
        if (!BusyUART1())
        {
            WriteUART1('1');
        }
        for (i = 0; i < 0xFFFF; i++)
        {
        }
        LATDbits.LATD2 = 0;
        if (!BusyUART1())
        {
            WriteUART1('0');
        }
        for (i = 0; i < 0xFFFF; i++)
        {
        }
    }
    return;
}

