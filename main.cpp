/*
*	main.cpp
*
*	Created: 2019-04-03 오후 7:35:10
*	Author : Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose.
*
*/
#define  F_CPU 20000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#include "Music.h"
#include "AlarmWorks.h"
#include "Buttons.h"
#include "DisplayOut.h"
#include "MCP7940N_RTC.h"

#define _LED1 0b00100000; // LED1 - PB5


MCP7940n clockObj;
//ClockWorks clockObj(0, 0, 0, 8192);
DisplayOut displayObj(0, 8192);
MusicObj musicObj(8192);
AlarmWorks alarmObj;
Buttons buttonObj(8192);


bool alarmTimeSetMode = false;
bool alarmShowMode = false;
bool hour12Mode = false;
uint16_t alarmShowTicks = 0;

bool toggleDots = false;
uint16_t toggleDotsTicks = 0;

ISR(RTC_PIT_vect)
{
	displayObj.refreshDisplay();
	buttonObj.buttonTickCounter();
	musicObj.autoRoutine();
	
	RTC_PITINTFLAGS = (1<<0); // Clear Interrupt bit.
}

uint16_t randGen()
{
	static uint16_t in = 32354;
	in ^= in <<6;
	in ^= in >>9;
	in ^= in <<2;
	return in;
}


void inputFunction(uint8_t buttonReturn, uint16_t buttonCounts)
{
	if (buttonReturn == BUTTON_UNDER_TICKS)
	{
	}
	// If nothing was pressed - load time from RTC
	else if (buttonReturn == BUTTON_NULL)
	{
		clockObj.loadTime();
	}
	else
	{
		ClockWorks* tempObj;
		// Do different things depending on the mode.
		if (alarmTimeSetMode)
		{
			tempObj = &alarmObj;
		}
		else
		{
			tempObj = &clockObj;
		}
		// If alarm button was unpressed...
		if (buttonReturn == BUTTON_ALARM_UNPRESSED)
		{
			// If the has been triggered, reset the alarm.
			if (alarmObj.isTriggered())
			{
				alarmObj.unTriggerAlarm();
				musicObj.musicOff();
				toggleDots = false;
				displayObj.setDot(false, true);
			}
			// If the system is in alarm time setting mode, exit the mode & set the alarm.
			else if (alarmTimeSetMode)
			{
				displayObj.setDot(false, true);
				alarmTimeSetMode = false;
				alarmObj.setAlarm();
			}
			// If the button wasn't pressed long enough & if the alarm hasn't been triggered & if the system is in normal mode...
			else if (buttonCounts <12)
			{
				// If the alarm is set, unset alarm.
				if (alarmObj.isSet())
				{
					alarmObj.unSetAlarm();
				}
				// If not set, set alarm.
				else
				{
					alarmObj.setAlarm();
				}
				// Display alarm time for a brief period.
				alarmShowMode = true;
				displayObj.setDot(true, true); // Dots On
			}
			// If the button was pressed long enough & if the alarm hasn't been triggered & if the system is in normal mode...
			else
			{
				// Enter alarm time setting mode. (No matter whether the alarm is currently set or not.)
				displayObj.setDot(true, true); // Dots On
				alarmTimeSetMode = true;
				alarmObj.unSetAlarm();
			}
			// End of alarm button routine.
		}
		// If the button has been pressed only for a short period, only change time when the button is depressed.
		else if (buttonCounts < 5)
		{
			if (buttonReturn == BUTTON_UP_UNPRESSED)
			{
				tempObj->addMin();
			}
			else if(buttonReturn == BUTTON_DOWN_UNPRESSED)
			{
				tempObj->subMin();
			}
		}
		// If the button has been pressed for a  while, keep changing time while the button is pressed.
		else
		{
			if (buttonReturn == BUTTON_UP_PRESSED)
			{
				tempObj->addMin();
			}
			else if(buttonReturn == BUTTON_DOWN_PRESSED)
			{
				tempObj->subMin();
			}
		}
		// Save time to RTC when the button is unpressed & if the clock time was the one being changed.
		if (tempObj == &clockObj)
		{
			if (buttonReturn == BUTTON_UP_UNPRESSED || buttonReturn == BUTTON_DOWN_UNPRESSED)
			{
				clockObj.saveTimeSecZero();
			}
		}
	}
}



int main(void)
{
	//System Clock Setup
	CPU_CCP = 0xD8; // Unprotects IO Reg.
	CLKCTRL_MCLKCTRLA = 0b00000000; // 20Mhz Selected.
	CPU_CCP = 0xD8; // Unprotects IO Reg.
	CLKCTRL_MCLKCTRLB = 0b00000000; // No Clock Prescaler.
	
	//PORT Direction Setup
	PORTA_DIR = 0b11111110; // Num 0, Anode7 ~ Anode1
	PORTB_DIR = 0b00111100; // Num 1, CDS(Used as LED1), LED 1(Used as Num 3), Buzzer, SW3/SDA, SW2/SCL
	PORTC_DIR = 0b00001110; // Num 2, 595 Latch clock, 595 Serial Clock, SW1/595 Serial Data
	
	PORTA_OUT = 0;
	PORTB_OUT = 0;
	PORTC_OUT = 0;
	
	//Internal RTC Setup
	RTC_CLKSEL = 0b00000000; // Use internal low power 32.768kHz oscillator.
	RTC_PITINTCTRL = 0b00000001; // PIT Interrupt enabled
	RTC_PITCTRLA = 0b00001001; // PIT Period 4 cycles. PIT Enabled. 8192Hz Interrupts.
	
	
	alarmTimeSetMode = false;
	sei();
	
	bool bootUp = true;
	bool secMode = false;
	hour12Mode = true;
	
	// BOOTUP SEQUENCE
	toggleDotsTicks = 0;
	while (bootUp)
	{
		DisplayOut temp(1, 8192);
		toggleDotsTicks++;
		buttonObj.buttonFunction(temp);
		if (toggleDotsTicks < 150)
		{
			// Display bootup screen #1 (1.04859)
			displayObj.setDisplayBootUp(0);
			// Enter 24 hour display mode when any of the buttons were pressed.
			if (buttonObj.getButtonPressed() != BUTTON_NULL && buttonObj.getButtonPressed() != BUTTON_UNDER_TICKS)
			{
				hour12Mode = false;
			}
			// Enter Seconds display mode when Alarm button is pressed until the end of bootup screen #1
			if (buttonObj.getButtonPressed() == BUTTON_ALARM_PRESSED)
			{
				secMode = true;
			}
			else if (buttonObj.getButtonPressed() != BUTTON_UNDER_TICKS)
			{
				secMode = false;
			}
		}
		else if (toggleDotsTicks < 250)
		{
			// Display bootup screen #1 (Blank)
			displayObj.setDisplayBootUp(1);
		}
		else
		{
			bootUp = false;
			clockObj.loadTime();
		}
		
		// SecMode indication LED
		if (secMode)
		{
			PORTB_OUTSET = _LED1; // LED1 On
		}
		else
		{
			PORTB_OUTCLR = _LED1; // LED1 Off
		}
		
		displayObj.animationRoutin();
		_delay_ms(10);
	}
	
	while (1)
	{
		// Button Controls
		uint8_t oldSec = clockObj.getSec();
		buttonObj.buttonFunction(displayObj);
		inputFunction(buttonObj.getButtonPressed(), buttonObj.getButtonCounter());
		if (toggleDots)
		{
			toggleDotsTicks++;
			if (toggleDotsTicks > 1000)
			{
				displayObj.setDot(false, true);
				toggleDotsTicks = 0;
			}
			else if (toggleDotsTicks > 500)
			{
				displayObj.setDot(true, true);
			}
		}
		
		// Alarm & Music Controls
		if(alarmObj.checkAlarm(clockObj))
		{
			musicObj.musicOn();
			toggleDots = true;
		}
		
		musicObj.musicFunction();
		
		// Alarm indication LED toggler
		if (alarmObj.isSet())
		{
			PORTB_OUTSET = _LED1; // LED1 On
		}
		else
		{
			PORTB_OUTCLR = _LED1; // LED1 Off
		}

		// Seconds Dot toggler
		static uint16_t mills = 0;
		mills++;
		if (oldSec < clockObj.getSec())
		{
			mills = 0;
		}
		if (mills > 500)
		{
			displayObj.setDot(3, true, true);
		}
		else
		{
			displayObj.setDot(3, false, true);
		}
		
		// Display Output Selection
		if (alarmTimeSetMode)
		{
			displayObj.setDisplay(alarmObj, hour12Mode);
		}
		// when displaying only for a short period of time.
		else if (alarmShowMode)
		{
			displayObj.setDisplay(alarmObj, hour12Mode);
			alarmShowTicks++;
			if (alarmShowTicks > 750)
			{
				alarmShowTicks = 0;
				alarmShowMode = false;
				displayObj.setDot(false, true);
			}
		}
		else
		{
			displayObj.setDisplay(clockObj, hour12Mode);
			
			if (secMode)
			{
				displayObj.setDisplayNoAnimationSecs(clockObj);
			}
		}
		_delay_ms(1);
	}
	
}
