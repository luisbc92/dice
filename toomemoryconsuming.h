#include <msp430.h>
#include <stdint.h>

// Definitions
#define PWM_LEVELS	16

// 1MHz / div
#define DIV_NORM	1000
#define DIV_LOWP	1000

#define ADC_SHT		ADC10SHT_3
#define ADC_SR		ADC10SR
#define ADC_DIV		ADC10DIV_0

#define ACC_TRESH	80
#define ACC_OFFX	0
#define ACC_OFFY	0
#define ACC_OFFZ	0
#define AX			0
#define AY			1
#define AZ			2

/*
 * LEDs
 */
typedef struct Led {
	uint8_t *dir;
	uint8_t *out;
	uint8_t bit;
} Led;

const Led led[] = {
		{(uint8_t *)&P1DIR, (uint8_t *)&P1OUT, BIT7},	// Face 1
		{(uint8_t *)&P1DIR, (uint8_t *)&P1OUT, BIT3},	// Face 2
		{(uint8_t *)&P1DIR, (uint8_t *)&P1OUT, BIT2},	// Face 3
		{(uint8_t *)&P2DIR, (uint8_t *)&P2OUT, BIT6},	// Face 4
		{(uint8_t *)&P1DIR, (uint8_t *)&P1OUT, BIT0},	// Face 5
		{(uint8_t *)&P1DIR, (uint8_t *)&P1OUT, BIT1}		// Face 6
};

uint8_t ledval[6];

void ledInit() {
	// Initialize ports for leds
	uint8_t i;
	for (i=0; i < 6; i++) {
		//*led[i].out &= ~(led[i].bit);
		//*led[i].dir |= led[i].bit;
	}
}

/*
 * Accelerometer
 */
int16_t acc[3];	// Accelerometer Axis Data

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
	// Faces
	if (acc[AZ] < ACC_TRESH) return 0;
	if (acc[AY] > ACC_TRESH) return 1;
	if (acc[AX] > ACC_TRESH) return 2;
	if (acc[AX] < ACC_TRESH) return 3;
	if (acc[AY] > ACC_TRESH) return 4;
	if (acc[AZ] > ACC_TRESH) return 5;
	return 0;
}


int main() {
	// Initialize System Clocks
	WDTCTL = WDTPW + WDTHOLD;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	// Initialize Ports
	P1OUT = 0;
	P2OUT = 0;
	P1DIR = 0;
	P2DIR = 0;

	// Initialize Peripherals
	//ledInit();
	//accInit();

	// Initialize Timer
	TACCR0 = DIV_NORM;						// 1MHz / TACCR0
	TACTL = TASSEL_2 | ID_0 | MC_1 | TAIE;	// SMCLK, Div0, Up, Interrupt Enable

	// Enable interrupts and enter LPM0
	__bis_SR_register(CPUOFF + GIE);

	while (1) {
		//static uint8_t count;
		static int i;
		i += led[0].bit;
		//count++;
		//if (count > 64) {
		//ledval[0] = 1;
		//	count = 0;
		//}

		//if (ledval[0] > 16) ledval[0] = 0;

		//__delay_cycles(1000);

		__bis_SR_register(CPUOFF);

	}

	//return 0;
}

/*
 * PWM and ADC triggering
 */
#pragma vector = TIMER0_A1_VECTOR
__interrupt void timerUpdate() {
	// PWM
	/*static uint8_t pwm = 0;
	uint8_t i;
	for (i=0; i < 6; i++) {
		if (ledval[i] > pwm) {
			*led[i].out |= led[i].bit;
		} else {
			*led[i].out &= ~led[i].bit;
		}
	}
	pwm++;
	if (pwm == PWM_LEVELS) pwm = 0;*/

	// Trigger ADC conversion
	//ADC10SA = (uint16_t) acc;		// Buffer pointer
	//ADC10CTL0 |= ENC + ADC10SC;		// Start conversions

	// Exit LPM0
	__bic_SR_register_on_exit(CPUOFF);
}

/*
 * ADC10 Sample Finished
 */
#pragma vector = ADC10_VECTOR
__interrupt void accEvent() {
	acc[AX] += ACC_OFFX;
	acc[AY] += ACC_OFFY;
	acc[AZ] += ACC_OFFZ;

	// Exit LPM0
	__bic_SR_register_on_exit(CPUOFF);
}
