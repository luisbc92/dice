
#include <msp430.h> 
#include "adxl345.h"

/*
 * Definitions
 */
#define ACC_MAX		255		// Accelerometer at 1G
#define ACC_TRESH	55		// Treshold to detect face
#define PWM_FADE	20		// Fade period in ms

/*
 * Typedefs
 */
typedef enum State {
	OFF,
	ROLLING,
	FACE
} State;

/*
 * Global Variables
 */
unsigned long timer;		// Timer
char pwm[6];				// PWM Channels'
char pwm_fade;				// PWM Fade (Trigger)
char face;					// Current face up

/*
 * Dice
 */
char readFace() {
	int acc[3];
	ADXL345ReadAcc(acc);

	// X+
	if (acc[0] > ACC_MAX - ACC_TRESH) {
		return 0;
	}

	// X-
	if (acc[0] < -(ACC_MAX - ACC_TRESH)) {
		return 1;
	}

	// Y+
	if (acc[1] > ACC_MAX - ACC_TRESH) {
		return 2;
	}

	// Y-
	if (acc[1] < -(ACC_MAX - ACC_TRESH)) {
		return 3;
	}

	// Z+
	if (acc[2] > ACC_MAX - ACC_TRESH) {
		return 4;
	}

	// Z-
	if (acc[2] < -(ACC_MAX - ACC_TRESH)) {
		return 5;
	}

	return 6;
}

/*
 * LEDs
 */
void ledOn(char led) {
	char i = 6;
	while (i--) {
		if (led == i) pwm[i] = 10;
		else pwm[i] = 0;
	}
}

void ledOff() {
	char i = 6;
	while (i--) {
		pwm[i] = 0;
	}
}

/*
 * Main
*/
void main(void) {
	
	// Disable Watchdog
    WDTCTL = WDTPW | WDTHOLD;

    // Configure MCLK @ 1MHz
	BCSCTL2 = SELM_0 + DIVM_0 + DIVS_0;
	DCOCTL = 0x00;
	BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
	DCOCTL = CALDCO_1MHZ;

	// Timer 0 (PWM / Timer)
	TA0CCTL0 = CM_3 | CCIS_0 | OUTMOD_0 | CCIE;		// Compare mode
	TA0CCR0 = 1000;									// (1MHz / 1000 = 1000 Hz)
	TA0CTL = TASSEL_2 | ID_0 | MC_1;				// SMCLK, DIV1, up mode

	// Configure LED Port
	P2DIR = 0xFF;
	P2OUT = 0x00;

	// Enable interrupts
	_bis_SR_register(GIE);

	// Initialize ADXL345
	ADXL345Init();
	ADXL345Write(POWER_CTL, (1 << MEASURE));

	// Dice
	static State state;
	static char lastFace;

	while (1) {
		pwm_fade = 1;
		ledOn(readFace());
		/*switch (state) {
		case OFF:
			ledOff();
			if (readFace() != lastFace) {
				state = ROLLING;
				lastFace = readFace();
			}
			break;

		case ROLLING:
			pwm_fade = 1;
			ledOn(readFace());
			if (readFace() == lastFace && timer > 100) {
				state = FACE;
			} else {
				lastFace = readFace();
			}
			break;

		case FACE:
			pwm_fade = 1;
			int i = 6;
			while (i--) {
				if (i % 2)
					ledOn(lastFace);
				else
					ledOff();
				timer = 0;
				while (timer < 3000);
			}
		}*/
	}
}

/*
 * Timer 0 Interrupt
 * 1000Hz
 *
 * PWM / Timer
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
	// PWM
	static char fader = 0;
	static char _pwm[6];
	static char cycle = 0;
	char i = 6;

	while (i-- > 0) {
		// Fader
		if (pwm_fade) {
			if (fader >= PWM_FADE) {
				if (_pwm[i] < pwm[i]) _pwm[i]++;
				if (_pwm[i] > pwm[i]) _pwm[i]--;
			}
		} else {
			_pwm[i] = pwm[i];
		}

		// PWM
		if (_pwm[i] > cycle)
			P2OUT |= (1 << i);
		else
			P2OUT &= ~(1 << i);
	}

	fader++;
	cycle++;
	if (fader > PWM_FADE) fader = 0;
	if (cycle == 16) cycle = 0;

	// Timer
	timer++;
}















