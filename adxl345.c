/*
 * adxl345.c
 *
 *  Created on: Mar 14, 2014
 *      Author: Luiz
 */

/*
 * Includes
 */
#include <msp430g2452.h>
#include "adxl345.h"

/*
 * Variables
 */
int ofs[3];

/*
 * Functions
 */
char spiTransfer(char data) {
	USISRL = data;					// Transmit data
	USICNT = 8;						// 8 Bits
	while (!(USIIFG & USICTL1));	// Wait for UCA0RXBUF to be free
	return USISRL;
}

void ADXL345Init() {
	// Configure Port for SPI
	P1OUT &= ~(MASK);				// Clear port
	P1SEL |= CLK + MISO + MOSI;		// Connect SPI
	P1DIR |= CS;					// Outputs
	P1OUT |= CS;					// Disable ADXL345

	// Configure interrupts
	P1IES &= ~(INT1 + INT2);	// Rising-edge interrupt
	P1IFG &= ~(INT1 + INT2);	// Clear interrupt flag
	P1IE = INT1 + INT2;			// Enable interrupts

	// Configure USCI_A0 as SPI @ 500 kbps
	// Master, MSB, Clock inactive state high
	// Data capture on following UCLK edge
	USICTL0 = USIPE7 + USIPE6 + USIPE5 + USIMST + USIOE + USISWRST;
	USICKCTL = USIDIV_1 | USISSEL_2 | USICKPL;
	USICTL0 &= ~USISWRST;

	// Configure DATA_FORMAT
	ADXL345Write(DATA_FORMAT, 0);

	// Configure offsets
	ofs[0] = 16;
	ofs[1] = -1;
	ofs[2] = -36;
}

void ADXL345Write(char address, char data) {
	P1OUT &= ~CS;			// Enable
	spiTransfer(address);	// Address
	spiTransfer(data);		// Data
	P1OUT |= CS;			// Disable
}

void ADXL345Read(char address, int num, char *data) {
	// Set MSB to 1 to signal a read operation
	char addr = 0x80 | address;
	// If doing multi-byte read, set bit 6
	if (num > 1) addr = addr | 0x40;

	P1OUT &= ~CS;		// Enable
	spiTransfer(addr);		// Address
	int i;
	for (i = 0; i < num; i++) {
		data[i] = spiTransfer(0x00);	// Read
	}
	P1OUT |= CS;		// Disable
}

void ADXL345ReadAcc(int *data) {
	char read[6];
	ADXL345Read(DATAX0, 6, read);
	data[0] = (int) ((read[1] << 8) | read[0]) - ofs[0];
	data[1] = (int) ((read[3] << 8) | read[2]) - ofs[1];
	data[2] = (int) ((read[5] << 8) | read[4]) - ofs[2];
}

void ADXL345Calibrate() {
	ADXL345ReadAcc(ofs);
	ofs[2] = ofs[2] - 256;
}











