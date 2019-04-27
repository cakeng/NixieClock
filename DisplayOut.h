/*
*	DisplayOut.h
*
*	Created: 2018-07-10 오전 6:43:41
*	Author: Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose.
*
*
*	2019.04.03 - Modified for the Nixie816 Clock.
*	All animation related functions that were used in the 7-segment based displays are bypassed.
*
*	Pin infos
*	PORTA (PA7 to PA1) - Num 0, Anode6 ~ Anode1
*	PORTB (PB5 to PB0) - LED/CDS, Num 1, Num 2, Buzzer, SW3/SDA, SW2/SCL
*	PORTC (PC3 to PC0) - Num 3, 595 Serial Clock, 595 Latch clock, SW1/595 Serial Data
*	595	  (SO7 to SO0) - PR, PL, Num 9 ~ Num 4
*/

#ifndef DISPLAYOUT_CAKENG_H
#define DISPLAYOUT_CAKENG_H
#include <avr/io.h>

#include "ClockWorks.h"
#include "MCP7940N_RTC.h"

//
// Blanking intervals are required to reduce ghosting of the nixies. Each digit is turned on for (BLKINTV-1)/Freq seconds.
// BLKINTV value of 8 is found to be the brightest while reducing most of the ghosting effects at the voltage of roughly 210V.
// BLKINTV value can be reduced to further suppress the effects of ghosting, Although to compensate for the dimmer display,
// measures such as increasing the supply voltage or encasing the display in a box /w filters should be applied.
//
#define BLKINTV 8

class DisplayOut
{
	private:
	uint8_t brighnessConst[3];
	uint8_t displayConst;
	volatile uint8_t outNum;
	
	uint8_t segDataBuffer[6];
	uint8_t screenData[6];

	uint16_t animationTicks[6];
	uint16_t animationRateConst[4];
	uint16_t animationRate[6];
	uint8_t animationCount[6];
	bool animationOn;
	
	public:
	DisplayOut(uint8_t cons, uint16_t freq);
	void shiftOut(uint8_t data);
	void refreshDisplay();
	void setBrighness(uint8_t cons);
	
	void setDisplay(ClockWorks& clockObj, bool hour12Mode);
	void setDisplay(ClockWorks& clockObj);
	void setDisplaySecs(ClockWorks& clockObj);
	void setDisplay(uint8_t h, uint8_t m, uint8_t s);
	void setDisplay(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f);
	void setDisplay(uint8_t a);
	void setDisplayBootUp(uint8_t num);
	void setDisplayNoAnimation(ClockWorks& clockObj);
	void setDisplayNoAnimationMills(ClockWorks& clockObj);
	void setDisplayNoAnimationSecs(ClockWorks& clockObj);
	void setDisplayNoAnimation(uint16_t dat);
	void setDisplayNoAnimation(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f);
	void setDot(uint8_t digit, bool set, bool right);
	void setDot(bool set, bool right);
	
	void animationTickCounter();
	void animationRoutin();
	void animationFasterSpeed()
	{
		animationRate[2] = animationRateConst[3];
		animationRate[3] = animationRateConst[2];
		animationTicks[2] = 0;
		animationTicks[3] = 0;
	}
	void animationFastSpeed()
	{
		animationRate[3] = animationRateConst[1];
		animationTicks[3] = 0;
	}
	void animationNormalSpeed()
	{
		animationRate[2] = animationRateConst[0];
		animationRate[3] = animationRateConst[0];
		animationTicks[2] = 0;
		animationTicks[3] = 0;
	}
};

#endif