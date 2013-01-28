//
// http://www.atmel.com/Images/doc0952.pdf
//

#include <avr/io.h>

	.global UartTX

#define UARTDELAY	135			// 9600 @ 8Mhz 

#define UARTPORT	0x18
#define UARTPIN		0x16
#define	TxD			4			// Transmit pin 

;***** Global register variables


#define	Txbyte	R24			// Data to be transmitted
#define	bitcnt	R20			// Bit counter
#define	temp	R21			// Temporary storage register

;***************************************************************************
;*
;* "putchar"
;*
;* This subroutine transmits the byte stored in the "Txbyte" register
;* The number of stop bits used is set with the sb constant
;*
;* Number of words	:14 including return
;* Number of cycles	:Depens on bit rate
;* Low registers used	:None
;* High registers used	:2 (bitcnt,Txbyte)
;* Pointers used	:None
;*
;***************************************************************************

UartTX:	
	ldi		bitcnt,10		// 10 bits in total to send
	com		Txbyte			// Invert everything
	sec						// Start bit

putchar0:	
	brcc	putchar1		// If carry set
	sbi		UARTPORT,TxD		//    send a '0'
	rjmp	putchar2		//else	

putchar1:	
	cbi		UARTPORT,TxD		//    send a '1'
	nop

putchar2:
	rcall	UART_delay		// One bit delay
	rcall	UART_delay

	lsr		Txbyte			// Get next bit
	dec		bitcnt			// If not all bit sent
	brne	putchar0		//   send next
							// else
	ret						//   return



;***************************************************************************
;*
;* "UART_delay"
;*
;* This delay subroutine generates the required delay between the bits when
;* transmitting and receiving bytes. The total execution time is set by the
;* constant "b":
;*
;*	3·b + 7 cycles (including rcall and ret)
;*
;* Number of words	:4 including return
;* Low registers used	:None
;* High registers used	:1 (temp)
;* Pointers used	:None
;*



UART_delay:	
	ldi		temp,UARTDELAY
UART_delay1:	
	dec		temp
	brne	UART_delay1
	ret

