/*
 * PiGate-Microcontroller.c
 *
 * Created: 7/10/2019 11:09:57 AM
 * Author : AB0TJ
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define PTT_STATE_INITIALIZED	0x01
#define PTT_STATE_ACTIVE		0x02
#define PTT_STATE_TIMEOUT		0x04

struct Ptt
{
	uint16_t timeout;
	uint16_t timer;
	uint8_t state;
};

enum SpiState { Spi_Idle, Spi_Wait_Byte, Spi_Wait_Word1, Spi_Wait_Word2 };

enum SpiCommand
{
	Cmd_Reset = 0x00,
	Cmd_GetVersion = 0x01,
	Cmd_Ping = 0x02,
	Cmd_InitPtt0 = 0x10,
	Cmd_InitPtt1 = 0x11,
	Cmd_InitPtt2 = 0x12,
	Cmd_StatusPtt0 = 0x20,
	Cmd_StatusPtt1 = 0x21,
	Cmd_StatusPtt2 = 0x22,
	Cmd_SetPtt0 = 0x30,
	Cmd_SetPtt1 = 0x31,
	Cmd_SetPtt2 = 0x32,
	Cmd_ClearPtt0 = 0x40,
	Cmd_ClearPtt1 = 0x41,
	Cmd_ClearPtt2 = 0x42,
	Cmd_GetAdc0 = 0x80,
	Cmd_GetAdc1 = 0x81,
	Cmd_Idle = 0xFF
};

volatile struct Ptt ptt[3];
volatile uint8_t spiDone, spiIn, spiOut;
enum SpiCommand currentSpiCommand;
enum SpiState spiState;
const uint16_t version = 1;

void handleSpi();

int main(void)
{	
	// Set up pins
	DDRA = 1<<DDA2 | 1<<DDA5;		// PTT0, MISO are outputs
	DDRB = 1<<DDB0 | 1<<DDB2;		// PTT1 and PTT2 are outputs
	
	// Set up SPI
	USICR = 1<<USISIE | 1<<USIOIE | 1<<USIWM0 | 1<<USICS1; // SPI mode 0, start and overflow interrupts enabled
	USISR = 1<<USIOIF;
	spiIn = spiOut = Cmd_Idle;
	currentSpiCommand = Cmd_Idle;
	spiState = Spi_Idle;
	
	// Set up timers
	TCCR0B = 1<<CS01 | 1<<CS00;				// Timer0 normal mode, /64 clock
	TCCR1B = 1<<WGM12 | 1<<CS11 | 1<<CS10;	// Timer1 CTC mode, /64 clock
	OCR1A = (F_CPU / 64) - 1;				// 1Hz rate
	TIMSK1 = 1<<OCIE1A;						// Timer1 compare match interrupt enabled
	
	// Set up ADC
	DIDR0 = 1<<ADC1D | 1<<ADC0D;	// Disable digital inputs on ADC0 and ADC1 pins
	ADMUX = 1<<REFS1;				// Internal 1.1V reference
	ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADPS1 | 1<<ADPS0;	// Enable ADC, start conversion (to initialize), /8 clock prescaler (125kHz)
	
	// Set up pin change interrupts
	GIMSK = 1<<PCIE1 | 1<<PCIE0;
	PCMSK0 = 1 << PCINT3 | 1<<PCINT7;
	PCMSK1 = 1<<PCINT9;
	sei();
	
    while (1) 
    {
		// Put the CPU to sleep and wait for interrupts
		//set_sleep_mode(SLEEP_MODE_IDLE);
		//sleep_mode();
		
		// Handle any pending SPI events
		if (spiDone != 0)
		{
			handleSpi();
			spiDone = 0;
		}
    }
}

void initPtt(uint8_t p, uint16_t timeout)
{
	ptt[p].timeout = timeout;
	ptt[p].timer = 0;
	ptt[p].state = PTT_STATE_INITIALIZED;
}

void resetPtts()
{
	PORTA &= ~(1<<PORTA2);
	PORTB &= ~(1<<PORTB2 | 1<<PORTB2);
	ptt[0].state = ptt[1].state = ptt[2].state = 0;
	ptt[0].timeout = ptt[1].timeout = ptt[2].timeout = 0;
	ptt[0].timer = ptt[1].timer = ptt[2].timer = 0;
}

void setPtt(uint8_t p)
{
	if (!(ptt[p].state & PTT_STATE_INITIALIZED)) return;
	
	switch (p)
	{
		case 0:
			PORTA |= 1<<PORTA2;
			break;
		case 1:
			PORTB |= 1<<PORTB0;
			break;
		case 2:
			PORTB |= 1<<PORTB2;
			break;
	}
	
	ptt[p].timer = 0;
	ptt[p].state |= PTT_STATE_ACTIVE;
	ptt[p].state &= ~PTT_STATE_TIMEOUT;
}

void clearPtt(uint8_t p)
{
	switch (p)
	{
		case 0:
			PORTA &= ~(1<<PORTA2);
			break;
		case 1:
			PORTB &= ~(1<<PORTB0);
			break;	
		case 2:
			PORTB &= ~(1<<PORTB2);
			break;
	}
	
	ptt[p].state &= ~PTT_STATE_ACTIVE;
}

void spiWrite(uint8_t b)
{
	while ((USISR & 0x0F) != 0);	// Wait if shift is in progress
	USIDR = b;
}

void startAdc(uint8_t adc)
{
	ADMUX = (1<<REFS1) | adc;	// Set ADC mux
	ADCSRA |= 1<<ADSC;			// Start a conversion
	while (ADCSRA & 1<<ADSC);	// Wait for conversion to complete
	spiWrite(ADCL);				// Send low byte
	spiOut = ADCH;				// Queue the high byte
}

void handleSpi()
{
	static uint16_t temp;
	
	switch (spiState)
	{
		case Spi_Idle:			// First byte: command
			switch (spiIn)
			{
				case Cmd_Idle:	// Nothing
					break;
					
				case Cmd_Reset:	// Reset running config
					resetPtts();
					break;
					
				case Cmd_GetVersion:	// Return firmware version
					spiWrite(version);
					spiOut = version >> 8;
					break;
					
				case Cmd_Ping:			// Send a magic string to show we're alive
					spiWrite(0x55);
					spiOut = 0xAA;
					break;
					
				case Cmd_InitPtt0:		// Initialize PTT
				case Cmd_InitPtt1:
				case Cmd_InitPtt2:
					currentSpiCommand = spiIn;
					spiState = Spi_Wait_Word1;
					break;
					
				case Cmd_StatusPtt0:	// Send PTT status
				case Cmd_StatusPtt1:
				case Cmd_StatusPtt2:
					spiWrite(ptt[spiIn & 0x0F].state);
					break;

				case Cmd_SetPtt0:		// Activate PTT
				case Cmd_SetPtt1:
				case Cmd_SetPtt2:
					setPtt(spiIn & 0x0F);
					break;
					
				case Cmd_ClearPtt0:		// Deactivate PTT
				case Cmd_ClearPtt1:
				case Cmd_ClearPtt2:
					clearPtt(spiIn & 0x0F);
					break;
					
				case Cmd_GetAdc0:		// Get ADC Value
				case Cmd_GetAdc1:
					startAdc(spiIn & 0x0F);
					break;
			}
			break;
			
		case Spi_Wait_Word1:
			temp = spiIn;
			spiState = Spi_Wait_Word2;
			break;
			
		case Spi_Wait_Word2:
			temp |= (uint16_t)(spiIn << 8);
			
			switch (currentSpiCommand)
			{
				case Cmd_InitPtt0:
				case Cmd_InitPtt1:
				case Cmd_InitPtt2:
					initPtt(currentSpiCommand & 0x0F, temp);
					break;
					
				default:
					break;
			}
			
			currentSpiCommand = Cmd_Idle;
			spiState = Spi_Idle;
			break;
			
		default:
			break;
	}
	
	spiIn = Cmd_Idle;
}

ISR(TIM1_COMPA_vect)
{
	if (ptt[0].timeout > 0 && (PORTA & (1<<PORTA2)))	// PTT0 is active
	{
		if (ptt[0].timer++ >= ptt[0].timeout)
		{
			PORTA &= ~(1<<PORTA2);
			ptt[0].state |= PTT_STATE_TIMEOUT;
			ptt[0].state &= ~PTT_STATE_ACTIVE;
		}
	}
	
	if (ptt[1].timeout > 0 && (PORTB & (1<<PORTB0)))	// PTT1 is active
	{
		if (ptt[1].timer++ >= ptt[1].timeout)
		{
			PORTB &= ~(1<<PORTB0);
			ptt[1].state |= PTT_STATE_TIMEOUT;
			ptt[1].state &= ~PTT_STATE_ACTIVE;
		}
	}
		
	if (ptt[2].timeout > 0 && (PORTB & (1<<PORTB2)))	// PTT2 is active
	{
		if (ptt[2].timer++ >= ptt[2].timeout)
		{
			PORTB &= ~(1<<PORTB2);
			ptt[2].state |= PTT_STATE_TIMEOUT;
			ptt[2].state &= ~PTT_STATE_ACTIVE;
		}
	}
}

ISR(PCINT0_vect)
{
	static uint8_t lastPINA = 0;
	uint8_t thisPINA = PINA;
	
	if ((lastPINA & (1<<PINA3)) && !(thisPINA & (1<<PINA3))) clearPtt(0);	// PTT0 high -> low
	else if ((thisPINA & (1<<PINA3)) && !(lastPINA & (1<<PINA3))) setPtt(0);	// PTT0 low->high
	
	if ((lastPINA & (1<<PINA7)) && !(thisPINA & (1<<PINA7))) clearPtt(1);	// PTT1 high -> low
	else if ((thisPINA & (1<<PINA7)) && !(lastPINA & (1<<PINA7))) setPtt(1);	// PTT1 low->high
	
	lastPINA = thisPINA;
}

ISR(PCINT1_vect)
{
	static uint8_t lastPINB = 0;
	uint8_t thisPINB = PINB;
	
	if ((lastPINB & (1<<PINB1)) && !(thisPINB & (1<<PINB1))) clearPtt(2);	// PTT2 high -> low
	else if ((thisPINB & (1<<PINB1)) && !(lastPINB & (1<<PINB1))) setPtt(2);	// PTT2 low->high
	
	lastPINB = thisPINB;
}

ISR(TIM0_OVF_vect)			// SPI byte timed out
{
	TIMSK0 = 0;				// Disable timer interrupt
	USISR = 0;				// Reset USR counter
	USISR = 1<<USISIF;		// Clear start condition interrupt flag
	USICR |= 1<<USISIE;		// Enable USI start interrupt
}

ISR(USI_STR_vect)			// USI start condition
{
	USICR &= ~(1<<USISIE);	// Disable start condition interrupt
	TCNT0 = 0;				// Reset timer0 count
	TIFR0 = 1<<TOV0;		// Clear timer0 interrupt flag
	TIMSK0 = 1<<TOIE0;		// Enable timer0 overflow interrupt
}

ISR(USI_OVF_vect)			// USI counter overflow
{
	TCNT0 = 0;				// Reset SPI timeout
	spiIn = USIDR;
	USIDR = spiOut;
	spiOut = Cmd_Idle;
	spiDone = 1;
	USISR = 1<<USIOIF;
}
