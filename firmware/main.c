#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "usbdrv.h"
#include "atmega-adc.h"

// utils
#define sbi(var, mask)	((var) |= (unsigned char) (1 << mask))
#define cbi(var, mask)	((var) &= (unsigned char)~(1 << mask))

// defines
#define ADC_CHANNELS 16
#define ADCDELAY 50		//50 μs to stabilize to the new value


typedef unsigned char  u08;
typedef   signed char  s08;
typedef unsigned short u16;
typedef   signed short s16;

// global
static uchar usb_reply[21];

/* ------------------------------------------------------------------------- */
/* ------------------------------- BOOTLOADER ------------------------------ */
/* ------------------------------------------------------------------------- */
void (*jump_to_bootloader)(void) = 0x7000; __attribute__ ((unused))

void startBootloader(void) {
		
		TIMSK0 &= ~(1<<TOIE0);			// disable timer overflow
	
		cli();							// turn off interrupts
		wdt_disable();					// disable watchdog timer
		usbDeviceDisconnect(); 			// disconnect from USB bus
		
		cbi(ADCSRA, ADIE);				// disable ADC interrupts
		cbi(ADCSRA, ADEN);				// disable ADC (turn off ADC power)
		
		PORTA = 0;						// pull all pins low
		PORTB = 0;
		PORTC = 0;						
		
		jump_to_bootloader();
}


// usbFunctionSetup
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
	usbRequest_t    *rq = (void *)data;

	usbMsgPtr = usb_reply;
	if(rq->bRequest == 0) { //POST
		return sizeof(usb_reply);

	} else if(rq->bRequest == 1) { //GET
		//rq->wIndex.bytes[0];
		//rq->wValue.bytes[0];
	}
	return 0;
}


// hardwareInit
static void hardwareInit(void)
{
	//USB
	uchar i, j;

	/* activate pull-ups except on USB lines */
	USB_CFG_IOPORT = (uchar) ~ ((1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT));

	/* all pins input except USB (-> USB reset) */
	#ifdef USB_CFG_PULLUP_IOPORT    /* use usbDeviceConnect()/usbDeviceDisconnect() if available */
	USBDDR = 0;        /* we do RESET by deactivating pullup */
	usbDeviceDisconnect();
	#else
	USBDDR = (1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT);
	#endif

	j = 0;
	while (--j) {        /* USB Reset by device only required on Watchdog Reset */
	i = 0;
	while (--i);    /* delay >10ms for USB reset */
	}

	#ifdef USB_CFG_PULLUP_IOPORT
	usbDeviceConnect();
	#else
	USBDDR = 0;        /*  remove USB reset condition */
	#endif

	// ENABLE ADC
	ADCSRA |= (1 << ADEN);
	// AUTO TRIGGER ENABLE
	//ADCSRA |= (0 << ADATE);
	// FREE RUNNING MODE
	//ADCSRB |= (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
	// AVCC with external capacitor at AREF pin
	//ADMUX |= (0 << REFS1) | (1 << REFS0);
	
	// RIGHT ADJUST (10 bits precision)
	ADMUX |= (0 << ADLAR);
	
	// Start a conversion
	ADCSRA |= (1 << ADSC); 
	
	// PORT F: ADC 0-7
	DDRF = 0x00;
	PORTF = 0x00;
	// PORT K: ADC 8-15
	DDRK = 0x00;
	PORTK = 0x00;
	
	// PORT C: INPUT / pull-up
	DDRC = 0x00;
	PORTC = 0xFF;
	
	/*
	// PORT x: OUTPUT
	DDRx = 0xFF;
	*/
	
	// LED
	DDRD |= (1 << 6);
	DDRD |= (1 << 7);
}

// main
int main(void) {
	
	wdt_enable(WDTO_1S);
	hardwareInit();
	usbInit();
	sei();
	
	unsigned int adcValue,replymask,replyshift,replybyte;
	
	while(1) {
		uchar i = 0;
		wdt_reset();
		usbPoll();
		PORTD ^= (1 << 6);
		
		//jump to bootloader if jumper is HIGH
		if(bit_is_clear(PIND, 5)) {
			startBootloader();
		}
		
		for(i = 0; i < ADC_CHANNELS; i++) {
			//adcValue = adc_read(ADC_PRESCALER_32, ADC_VREF_AVCC, i);
			adcValue = i;
			usb_reply[i] = adcValue >> 2;	
			replybyte = 16 + (i / 4);
			replyshift = ((i % 4) * 2);
			replymask = (3 << replyshift);
			usb_reply[replybyte] =	(usb_reply[replybyte] & ~replymask) | (replymask & (adcValue << replyshift));
			_delay_us(ADCDELAY);
		}
		usb_reply[20] = PINC;
	}
	return 0;
}
