/*
 * ADXL345.h
 *
 * Library for ADXL345
 *
 * Pins:
 * 	P1.2 -> INT2
 * 	P1.3 -> INT1
 * 	P1.4 -> CS
 * 	P1.5 -> CLK
 * 	P1.6 -> MOSI
 * 	P1.7 -> MISO
 *
 *
 *  Created on: Mar 14, 2014
 *      Author: Luis Bañuelos
 */

#ifndef ADXL345_H_
#define ADXL345_H_

/*
 * Includes
 */
#include <msp430.h>

/*
 * Definitions
 */

// Pins
#define INT2	BIT2
#define INT1	BIT3
#define CS		BIT4
#define CLK		BIT5
#define MOSI	BIT6
#define MISO	BIT7
#define MASK	(INT2+INT1+CS+CLK+MOSI+MISO)

// Registers
#define DEVID               (0x00)    // Device ID
#define THRESH_TAP          (0x1D)    // Tap threshold
#define OFSX                (0x1E)    // X-axis offset
#define OFSY                (0x1F)    // Y-axis offset
#define OFSZ                (0x20)    // Z-axis offset
#define DUR                 (0x21)    // Tap duration
#define LATENT              (0x22)    // Tap latency
#define WINDOW              (0x23)    // Tap window
#define THRESH_ACT          (0x24)    // Activity threshold
#define THRESH_INACT        (0x25)    // Inactivity threshold
#define TIME_INACT          (0x26)    // Inactivity time
#define ACT_INACT_CTL       (0x27)    // Axis enable control for activity and inactivity detection
#define THRESH_FF           (0x28)    // Free-fall threshold
#define TIME_FF             (0x29)    // Free-fall time
#define TAP_AXES            (0x2A)    // Axis control for single/double tap
#define ACT_TAP_STATUS      (0x2B)    // Source for single/double tap
#define BW_RATE             (0x2C)    // Data rate and power mode control
#define POWER_CTL           (0x2D)    // Power-saving features control
#define INT_ENABLE          (0x2E)    // Interrupt enable control
#define INT_MAP             (0x2F)    // Interrupt mapping control
#define INT_SOURCE          (0x30)    // Source of interrupts
#define DATA_FORMAT         (0x31)    // Data format control
#define DATAX0              (0x32)    // X-axis data 0
#define DATAX1              (0x33)    // X-axis data 1
#define DATAY0              (0x34)    // Y-axis data 0
#define DATAY1              (0x35)    // Y-axis data 1
#define DATAZ0              (0x36)    // Z-axis data 0
#define DATAZ1              (0x37)    // Z-axis data 1
#define FIFO_CTL            (0x38)    // FIFO control
#define FIFO_STATUS         (0x39)    // FIFO status
#define MULTIPLIER 			(0.004)   // 4mg per lsb

// Offsets

// ACT_INACT_CTL
#define ACT			(7)
#define ACT_X		(6)
#define ACT_Y		(5)
#define ACT_Z		(4)
#define INACT		(3)
#define INACT_X		(2)
#define INACT_Y		(1)
#define INACT_Z		(0)

// TAP_AXES / ACT_TAP_STATUS
#define SUPPRESS	(3)
#define TAP_X		(2)
#define TAP_Y		(1)
#define TAP_Z		(0)

// BW_RATE
#define LOW_POWER	(4)
#define RATE		(0)

// POWER_CTL
#define LINK		(5)
#define AUTO_SLEEP	(4)
#define MEASURE		(3)
#define SLEEP		(2)
#define WAKEUP		(0)

// INT
#define DATA_READY	(7)
#define SINGLE_TAP	(6)
#define DOUBLE_TAP	(5)
#define ACTIVITY	(4)
#define INACTIVITY	(3)
#define FREE_FALL	(2)
#define WATERMARK	(1)
#define OVERRUN		(0)

// DATA_FORMAT
#define SELF_TEST	(7)
#define SPI			(6)
#define INT_INVERT	(5)
#define FULL_RES	(3)
#define JUSTIFY		(2)
#define RANGE		(0)


/*
 * Functions
 */
void ADXL345Init();										// Initialize
void ADXL345Write(char address, char data);				// Write
void ADXL345Read(char address, int num, char *data);	// Read
void ADXL345ReadAcc(int *data);							// Read Axis Data
void ADXL345Calibrate();								// Calibrate



#endif /* ADXL345_H_ */
