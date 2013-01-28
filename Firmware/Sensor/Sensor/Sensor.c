/*
 * Sensor.c
 *
 * Created: 10/11/2012 23:29:35
 *  Author: Mats
 */ 

/*
    
	25 2a
	0010101010
	
	
		+--8-------7-------6-------5--+		
		| VCC     PB2     PB1     PB0 |		PB0=MOSI			PB3=ADC3
		>         attiny45/85         |		PB1=MISO			PB4=ADC2
		| PB5     PB3     PB4     GND |		PB2=SCK/ACD1/INT0	PB5=~RESET/ADC0
		+--1-------2-------3-------4--+


		PB2 = INT0 for zero crossing detector
		PB3 = ADC3 for line voltage measurement 
		PB4 = Output to optocoupler


	Data to be bitbanged to the optocoupler"

		First byte:		1  0  0  0  0 B9 B8 B7
		Second byte:	0 B6 B5 B4 B3 B2 B1 B0


*/

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h> 
#include <avr/power.h>

void UartTX(uint8_t chr);

#define SAMPLES	50

volatile uint16_t voltageMax;						// Max ADC reading during a 10mS power period
volatile uint16_t sum;								// Sum of all max readings during 1 second

uint8_t delaycnt=145;



//
// This ISR is triggered by the zero crossings every 10 or 8.33 mS (rising and 
// falling edges on a 50 or 60 Hz network)
//
ISR(INT0_vect) {
	static uint16_t mySum=0;
	static uint8_t sampleCount=0;
	
	mySum+=voltageMax;
	voltageMax=0;
	sampleCount++;
	// If we collected 50/60 max values then 1 second have passed
	// and we transfer the local sum to the global for transmission
	// by the main loop. Since the ADC interrupts now are disabled
	// the serial bit banging can be very precise. The sending must
	// be finalized before the next zero crossing. I.E in 10 mS.
	if (sampleCount==SAMPLES) {
		sum=mySum;
		mySum=0;
		sampleCount=0;
	}	
}



//
// Triggered by ADC conversion complete. Update the global variable voltageMax if the current 
// reading is higher than the previous readings for this cycle.
//
ISR(ADC_vect) { 
	uint8_t low;
	uint16_t result;
	
	low=ADCL;
	result=((ADCH<<8) | low);
	if (result>voltageMax) voltageMax=result;
}




int main(void) {
	uint8_t vh, vl;
	uint8_t i;
	
	clock_prescale_set(clock_div_1);		// 8 MHz system clock from internal RC

	DIDR0 |= (1<<ADC3D);					// Turn off digital input on ADC3
	DDRB=(1<<PB4);							// Set PB4 as output for optocoupler			
	PORTB=0;							

	for (i=0; i<50; i++) {					// Wait 5 seconds after power on for
		_delay_ms(100);						// things to stabilize
	}	


	// Ref=Internal 2.56V Voltage Reference without external bypass capacitor
	ADMUX =		(1<<REFS2) | (1<<REFS1) | (0<<REFS0) |	// ADC Voltage Reference
				(0<<ADLAR) |							// ADC Left Adjust Result
				(0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (1<<MUX0); // Analog Channel & Gain 

	// Start up ADC free running mode with /128 prescaler resulting in 625 samples
	// per 10mS cycle
	ADCSRA =	(1<<ADEN) |		// ADC Enable
				(1<<ADSC) |		// ADC Start Conversion
				(1<<ADATE) |	// ADC Auto Trigger Enable
				(0<<ADIF) |		// ADC Interrupt Flag (cleared by irq)
				(1<<ADIE) |		// ADC Interrupt Enable
				(1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // ADC Prescaler


	GIMSK=(1 << INT0);			// Enable External interrupts
	MCUCR |= (1<<ISC00);		// Any logical change on INT0 generates an interrupt request

	sei();						// Enable Global Interrupts

	for (;;) {
		while (sum==0xFFFF);

		sum=sum/50;
		vh=sum>>5;
		vl=sum&0b00011111;
		vh+=' ';
		vl+=' ';

		cli(); 
		UartTX(vh);
		UartTX(vl);
		UartTX(13);
		UartTX(10);
		sei();


		sum=0xFFFF;
	}		

}