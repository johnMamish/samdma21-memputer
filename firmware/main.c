#include "samd21g18a.h"
#include <stdint.h>

void init_hardware();

int main()
{

    uint8_t choffset = 0;
    while (1) {

    }
}


void init_hardware()
{
    // change OSC8M output divider from 8 to 1.
    SYSCTRL->OSC8M.reg &= ~(0b11ul << 8);

    // Adafruit metro uart tx pin is PA10 / SERCOM0/PAD[2]
    // select function C for pin PA10
    PORT->Group[0].PMUX[(10 >> 1)].reg &= ~(0x0f << 0);
    PORT->Group[0].PMUX[(10 >> 1)].reg |=  (0x02 << 0);
    PORT->Group[0].PINCFG[10].reg = 0x01;

    ////////////////////////////////////////////////////////////////////////////
    // Initialize sercom0 as a UART running at a 115200 baud off of OSC8M
    // enable sercom0's clock.
    PM->APBCMASK.reg |= (1 << 2);

    // route GCLK0 (which contains OSC8M) undivided to sercom0 and sercomx_slow
    GCLK->CLKCTRL.reg = (1 << 14) | (0 << 8) | (0x13 << 0);
    GCLK->CLKCTRL.reg = (1 << 14) | (0 << 8) | (0x14 << 0);
    SERCOM0->USART.CTRLA.reg = ((1 << 30) |    // DORD: LSB first
                                (3 << 20) |    // RXPO: SERCOM[3] is RX
                                (1 << 16) |    // TXPO: SERCOM[2] is TX
                                (1 << 13) |    // SAMPR: 16x oversamp w/ fractional baud gen
                                (1 << 2));     // use internal clock

    // Fractional division of 8MHz input clock by (16 * (4 + (3 / 8)).
    SERCOM0->USART.BAUD.reg = (3 << 13) | (4 << 0);

    // enable RX and TX
    SERCOM0->USART.CTRLB.reg = ((1 << 17) | (1 << 16));

    // enable SERCOM0
    SERCOM0->USART.CTRLA.reg |= (1 << 1);



}
