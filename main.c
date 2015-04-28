#include <msp430.h>
#include <stdint.h>

// PWM
#define PWM_LEVELS	127			// # of levels has to be (2^x-1)
#define PWM_MAX		64			// Maximum level allowed

// Timers
#define TIMER_NUM	6			// 6 timers (don't change)

// Loop frequency on normal and low power (1MHz / div)
#define DIV_NORM	125			// 8000Hz (Powers of 2)
#define DIV_LOWP	64000		// 15Hz

// Timer
#define TIMER_CYCPERMS_NORM		(1000/DIV_NORM-1)	// Cycles per millisecond on normal
#define TIMER_MSPERCYC_LOWP		(DIV_LOWP/1000)		// Milliseconds per cycle or low power

// ADC Configuration
#define ADC_SHT		ADC10SHT_3	// Long s&h time
#define ADC_SR		ADC10SR		// Rate limited
#define ADC_DIV		ADC10DIV_2	// --^

// Accelerometer Configuration (Per Dice Adjustment)
#define ACC_TRESH	20
#define ACC_OFFX	0
#define ACC_OFFY	0
#define ACC_OFFZ	0
#define AX			2
#define AY			1
#define AZ			0

// Dice
#define LED_IDLE_DELAY	750		// Led cycling delay when idle
#define LED_IDLE_RISE	500
#define LED_IDLE_FALL	2250
#define LED_ROLL_DELAY	100		// Led cycling delay when rolling
#define LED_ROLL_RISE	40
#define LED_ROLL_FALL	300
#define LED_BLINK_DELAY	1000	// Led blinking delay after rolling
#define LED_BLINK_RISE	400
#define LED_BLINK_FALL	400
#define SEQ_TIME		1500	// Time to complete each step of the poweron sequence
#define POWEROFF		15000	// Time before power off
#define RESET			5000	// Time before going to idle
#define ROLLING			300		// Time inbetween face changes to consider as roll

// Global Variables
uint16_t timer[TIMER_NUM];
uint8_t low_power = 0;

/*
 * LPM
 */
void inline lowpEnable() {		// Changes loop freq to DIV_LOWP
	low_power = 1;
	TACCR0 = DIV_LOWP;
}

void inline lowpDisable() {		// Changes loop freq to DIV_NORM
	low_power = 0;
	TACCR0 = DIV_NORM;
}

/*
 * Wait
 *
 * True when specified time has elapsed, non-blocking
 */
uint8_t wait(uint16_t ms, uint8_t num) {
	// Wait
	if (timer[num] > ms) {
		timer[num] = 0;
		return 1;
	}
	return 0;
}

void inline waitReset(uint8_t num) { timer[num] = 0; }

/*
 * LED
 */
uint8_t led[6];	// Brigthness of LED (0-PWM_LEVELS)

void ledInit() {
	P1DIR = BIT7 + BIT3 + BIT2 + BIT0 + BIT1;
	P2DIR = BIT6;
	P2SEL &= ~BIT6;
}

void inline ledSet(uint8_t num, uint8_t val) {
	switch (num) {
	case 0:
		(val) ? (P1OUT|=BIT7) : (P1OUT&=~BIT7);
		break;
	case 1:
		(val) ? (P1OUT|=BIT3) : (P1OUT&=~BIT3);
		break;
	case 2:
		(val) ? (P1OUT|=BIT2) : (P1OUT&=~BIT2);
		break;
	case 3:
		(val) ? (P2OUT|=BIT6) : (P2OUT&=~BIT6);
		break;
	case 4:
		(val) ? (P1OUT|=BIT0) : (P1OUT&=~BIT0);
		break;
	case 5:
		(val) ? (P1OUT|=BIT1) : (P1OUT&=~BIT1);
		break;
	}
}

/*
 * LED Effects
 */
void ledOff() {
	uint8_t i;
	for (i=0; i<6; i++) led[i] = 0;
}

void ledFadeOff(uint16_t period) {
	uint8_t i, j;
	for (i=0; i<PWM_MAX; i++) {
		while (wait(period/PWM_MAX, 0) == 0);
		for (j=0; j<6; j++) if (led[j] > 0) led[j]--;
	}
}

void ledCycle(uint16_t period, uint16_t risetime, uint16_t falltime) {
	static uint8_t num;
	static int8_t fadeon;
	uint8_t i;

	// Fade-off
	if (wait(falltime/PWM_MAX, 0)) {
		for (i=0; i<6; i++) {
			if (led[i] > 0 && fadeon != i) led[i]--;
		}
	}

	// Fade-on
	if (wait(risetime/PWM_MAX, 1)) {
		if (fadeon != -1 && led[fadeon] < PWM_MAX) {
			led[fadeon]++;
		} else {
			fadeon = -1;
		}
	}

	// Set fade-in
	if (wait(period, 2)) {
		fadeon = num++;
		if (num == 6) num = 0;
	}
}

void ledBlink(uint8_t num, uint16_t period, uint16_t risetime, uint16_t falltime) {
	static int8_t fadeon;
	uint8_t i;

	// Fade-off
	if (wait(falltime/PWM_MAX, 0)) {
		for (i=0; i<6; i++) {
			if (led[i] > 0 && fadeon != i)
				led[i]--;
		}
	}

	// Fade-on
	if (wait(risetime/PWM_MAX, 1)) {
		if (fadeon != -1 && led[fadeon] < PWM_MAX) {
			led[fadeon]++;
		} else {
			fadeon = -1;
		}
	}

	// Set fade-in
	if (wait(period, 2)) {
		fadeon = num;
	}
}

/*
 * Accelerometer
 */
int16_t accBuffer[3];	// Buffer for pre-shifting
int8_t acc[3];			// Accelerometer Axis Data

void accInit() {
	P1SEL |= BIT4 + BIT5 + BIT6;					// Connect inputs to ADC
	ADC10CTL0 = ADC_SHT + ADC_SR + MSC + ADC10ON + ADC10IE;	// MSC for DTC
	ADC10CTL1 = INCH_6 + CONSEQ_1 + ADC_DIV + ADC10DF;		// A6-4, MultiChannel Once, 2's complement
	ADC10AE0 = BIT4 + BIT5 + BIT6;					// Enable ADC inputs
	ADC10DTC1 = 3;									// Perform 3 conversions
}

/*
 * Dice
 */
uint8_t getTopFace() {
	static uint8_t curr;
	static uint8_t new;
	uint8_t face;

	// Get current face up
	if (acc[AX] > ACC_TRESH) face = 3;
	else if (acc[AY] > ACC_TRESH) face = 4;
	else if (acc[AZ] > ACC_TRESH) face = 5;
	else if (acc[AX] < -ACC_TRESH) face = 2;
	else if (acc[AY] < -ACC_TRESH) face = 1;
	else if (acc[AZ] < -ACC_TRESH) face = 0;

	// Filter sudden movements
	if (face == new) {
		if (wait(75, 3)) curr = face;
	} else {
		waitReset(3);
	}
	new = face;

	return curr;
}

/*
 * Main
 */
int main() {
	// Initialize System Clocks
	WDTCTL = WDTPW + WDTHOLD;
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	// Initialize Ports
	P1OUT = 0;
	P2OUT = 0;
	P1DIR = 0;
	P2DIR = 0;

	// Initialize Peripherals
	ledInit();
	accInit();

	// Initialize Timer
	TACCTL0 = CCIE;							// Interrupt on compare
	TACCR0 = DIV_NORM;						// 1MHz / TACCR0
	TACTL = TASSEL_2 | ID_3 | MC_1;			// SMCLK, Div0, Up

	// Enable interrupts and enter LPM0
	__bis_SR_register(CPUOFF + GIE);

	// State machine
	enum State {
		OFF,
		POWERON_SEQ_1,
		POWERON_SEQ_2,
		POWERON_SEQ_3,
		ON,
		ROLL,
		BLINK
	} state = ON;

	while(1) {
		static uint16_t faceChangeTime[3];
		static uint8_t faceLast;
		uint16_t faceChangeAvg;
		uint8_t face;
		uint8_t rolling;

		// Sample face and detect if rolling
		face = getTopFace();
		if (face != faceLast) {
			faceChangeTime[2] = faceChangeTime[1];
			faceChangeTime[1] = faceChangeTime[0];
			faceChangeTime[0] = timer[4];
			timer[4] = 0;
		}
		faceChangeAvg = (timer[4] + faceChangeTime[0] + faceChangeTime[1] + faceChangeTime[2]) >> 2;
		(faceChangeAvg < ROLLING) ? (rolling = 1) : (rolling = 0);

		switch (state) {
		case OFF:
			ledOff();
			lowpEnable();
			state++;
			break;

		case POWERON_SEQ_1:
			if (face == 0) state++;
			break;

		case POWERON_SEQ_2:
			if (face == 1) state++;
			if (wait(SEQ_TIME, 5)) state = POWERON_SEQ_1;
			break;

		case POWERON_SEQ_3:
			if (face == 2) {
				faceChangeTime[0] = ROLLING*2;
				faceChangeTime[1] = ROLLING*2;
				faceChangeTime[2] = ROLLING*2;
				timer[4] = ROLLING*2;
				lowpDisable();
				waitReset(5);
				state = ON;
			}
			if (wait(SEQ_TIME, 5)) state = POWERON_SEQ_1;
			break;

		case ON:
			ledCycle(LED_IDLE_DELAY, LED_IDLE_RISE, LED_IDLE_FALL);
			if (rolling) state = ROLL;
			if (wait(POWEROFF, 5)) {
				ledFadeOff(LED_IDLE_FALL);
				state = OFF;
			}
			break;

		case ROLL:
			ledCycle(LED_ROLL_DELAY, LED_ROLL_RISE, LED_ROLL_FALL);
			if (rolling == 0) {
				state = BLINK;
				waitReset(5);
			}
			break;

		case BLINK:
			ledBlink(face, LED_BLINK_DELAY, LED_BLINK_RISE, LED_BLINK_FALL);
			if (wait(RESET, 5)) {
				state = ON;
				waitReset(5);
			}
			break;
		}

		faceLast = face;
		__bis_SR_register(CPUOFF);
	}
}

/*
 * PWM and ADC triggering
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void timerUpdate() {
	// Cycle counter
	static uint8_t cycle;
	cycle++;

	// PWM
	uint8_t i;
	for (i=0; i < 6; i++) {
		if (led[i] > (cycle & PWM_LEVELS)) {
			ledSet(i, 1);
		} else {
			ledSet(i, 0);
		}
	}

	// Timers
	if (low_power == 0 && ((cycle & TIMER_CYCPERMS_NORM) == 0))
		for (i=0; i<TIMER_NUM; i++) timer[i]++;
	if (low_power == 1)
		for (i=0; i<TIMER_NUM; i++) timer[i]+= TIMER_MSPERCYC_LOWP;

	// Trigger ADC conversion
	if (low_power == 0 && (cycle & 127) == 0) {		// About 60Hz on normal mode
		ADC10SA = (uint16_t) accBuffer;		// Buffer pointer
		ADC10CTL0 |= ENC + ADC10SC;			// Start conversions
	} else if (low_power == 1) {
		ADC10SA = (uint16_t) accBuffer;		// Buffer pointer
		ADC10CTL0 |= ENC + ADC10SC;			// Start conversions
	}

	// Exit LPM0
	__bic_SR_register_on_exit(CPUOFF);
}

/*
 * ADC10 Sample Finished
 */
#pragma vector = ADC10_VECTOR
__interrupt void accEvent() {
	acc[AX] = (accBuffer[AX] >> 8) + ACC_OFFX;
	acc[AY] = (accBuffer[AY] >> 8) + ACC_OFFY;
	acc[AZ] = (accBuffer[AZ] >> 8) + ACC_OFFZ;
}



