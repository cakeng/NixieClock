/*
*	DisplayOut.cpp
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


#include "DisplayOut.h"

#define _595_LATCH 0b00000010
#define _595_CLOCK 0b00000100
#define _595_DATA 0b00000001


const uint8_t bootUpDisplayData[][6] =
{
	{	// 1.04859
		1,74,4,8,5,9
	},
	{
		32,32,32,32,32,32 // Blank
	}
};

// 7th bit & 6th bit is used to output the Right sided dot & Left sided Dot respectively.
const uint8_t numDataBox[] =
{
	10,//0
	1,//1
	2,//2
	3,//3
	4,//4
	5,//5
	6,//6
	7,//7
	8,//8
	9,//9
	0b00100000// Blank
};

void DisplayOut::setBrighness(uint8_t cons)
{
	displayConst = brighnessConst[cons]; // Off Time - Higher displayConst results in dimmer display.
}

DisplayOut::DisplayOut(uint8_t cons, uint16_t freq)
{
	brighnessConst[0] = 1;
	brighnessConst[1] = 18;
	brighnessConst[2] = 50;
	setBrighness(cons);
	outNum = 0;
	
	segDataBuffer[0] = 0;
	segDataBuffer[1] = 0;
	segDataBuffer[2] = 0;
	segDataBuffer[3] = 0;
	segDataBuffer[4] = 0;
	segDataBuffer[5] = 0;
	
	screenData[0] = 0;
	screenData[1] = 0;
	screenData[2] = 0;
	screenData[3] = 0;
	screenData[4] = 0;
	screenData[5] = 0;
	
	animationTicks[0] = 0;
	animationTicks[1] = 0;
	animationTicks[2] = 0;
	animationTicks[3] = 0;
	animationTicks[4] = 0;
	animationTicks[5] = 0;
	
	uint16_t buf = freq/100;
	animationRateConst[0] = 40*buf/10; // 40ms - Normal animation speed
	animationRateConst[1] = 16*buf/10; // 16ms - Fast animation speed.
	animationRateConst[2] = 2*buf/10; // 2ms - Fastest animation speed for minutes. (First digit)
	animationRateConst[3] = 12*buf/10; // 12ms - Fastest animation speed for minutes. (Second digit)
	
	animationRate[0] = animationRateConst[0];
	animationRate[1] = animationRateConst[0];
	animationRate[2] = animationRateConst[0];
	animationRate[3] = animationRateConst[0];
	animationRate[4] = animationRateConst[0];
	animationRate[5] = animationRateConst[0];
	
	animationCount[0] = 0;
	animationCount[1] = 0;
	animationCount[2] = 0;
	animationCount[3] = 0;
	animationCount[4] = 0;
	animationCount[5] = 0;
}

void DisplayOut::shiftOut(uint8_t data)
{
	PORTC_DIRSET = _595_DATA; // Set Data pin to output.
	PORTC_OUTCLR = _595_LATCH; // Latch pin Low
	for(uint8_t i = 0; i < 8; i++)
	{
		PORTC_OUTCLR = (_595_CLOCK|_595_DATA); // Clock/Data pin Low
		if (data&(128>>i))
		{
			PORTC_OUTSET = _595_DATA;
		}
		PORTC_OUTSET = _595_CLOCK; // Clock pin High
	}
	PORTC_OUTSET = _595_LATCH; // Latch pin High
	PORTC_OUTSET = _595_DATA;
	PORTC_DIRCLR = _595_DATA; // Set Data pin to input.
}

void DisplayOut::refreshDisplay()
{
	// If outNum < displayConst, the screen is off.
	// If displayConst < outNum < displayConst + (Number of digits)
	// Outputs (outNum - displayConsts)th digit.
	//
	// Blanking intervals are required to reduce ghosting of the nixies. Each digit is turned on for (BLKINTV-1)/Freq seconds.
	// BLKINTV value of 8 is found to be the brightest while reducing most of the ghosting effects at the voltage of roughly 210V.
	// BLKINTV value can be reduced to further suppress the effects of ghosting, Although to compensate for the dimmer display,
	// measures such as increasing the supply voltage or encasing the display in a box /w filters should be applied.
	// 
	outNum++;
	if (outNum < displayConst)
	{
	}
	else if (outNum < displayConst + (7*BLKINTV-1)) 
	{
		if ((outNum - displayConst)%BLKINTV)
		{
			uint8_t anodeNum = (outNum - displayConst)/BLKINTV;
			PORTA_OUTCLR = 0b11111110;
			PORTB_OUTCLR = 0b00011000;
			PORTC_OUTCLR = 0b00001000;
			shiftOut(0);
			PORTA_OUTSET = (64>>anodeNum);
			uint8_t curDat;
			if(animationOn)
			{
				curDat = screenData[anodeNum];
			}
			else
			{
				curDat = segDataBuffer[anodeNum];
			}
			uint8_t serialDat = curDat&0b11000000;
			uint8_t digitDat = curDat&0b00111111;
			if (curDat== 64 || curDat == 128)
			{
				digitDat = 32;
			}
			switch(digitDat)
			{
				case 32:
				break;
				case 10:
				PORTA_OUTSET = 0b10000000;
				break;
				case 1:
				PORTB_OUTSET = 0b00010000;
				break;
				case 2:
				PORTB_OUTSET = 0b00001000;
				break;
				case 3:
				PORTC_OUTSET = 0b00001000;
				break;
				default:
				serialDat |= (1<<(digitDat-4));
				break;
			}
			shiftOut(serialDat);
		}
		else
		{
			PORTA_OUTCLR = 0b11111110;
			PORTB_OUTCLR = 0b00011000;
			PORTC_OUTCLR = 0b00001000;
			shiftOut(0);
		}
	}
	else
	{
		PORTA_OUTCLR = 0b11111110;
		PORTB_OUTCLR = 0b00011000;
		PORTB_OUTCLR = 0b00001000;
		shiftOut(0);
		outNum = 0;
	}
}

void DisplayOut::setDisplay(ClockWorks& clockObj, bool hour12Mode)
{
	
	screenData[0] = 0;
	screenData[1] = 0;
	screenData[2] = 0;
	screenData[3] = 0;
	screenData[4] = 0;                
	screenData[5] = 0;
	
	if (hour12Mode)
	{
		uint8_t temp = clockObj.getHour();
		if (temp > 11)
		{
			screenData[1] |= 0x80; //PM
		}
		else
		{
			screenData[1] &= 0x7f; //AM
		}
		if (temp > 12)
		{
			temp -= 12;
		}
		else if (temp == 0)
		{
			temp = 12;
		}
		screenData[0] |= numDataBox[temp/10];
		screenData[1] |= numDataBox[temp%10];
	}
	else
	{
		screenData[0] |= numDataBox[clockObj.getHour()/10];
		screenData[1] |= numDataBox[clockObj.getHour()%10];
	}
	screenData[2] |= numDataBox[clockObj.getMin()/10];
	screenData[3] |= numDataBox[clockObj.getMin()%10];
	screenData[4] |= numDataBox[clockObj.getSec()/10];
	screenData[5] |= numDataBox[clockObj.getSec()%10];
	animationOn = true;
	
}
void DisplayOut::setDisplay(ClockWorks& clockObj)
{
	
	screenData[0] &= 0xC0;
	screenData[1] &= 0xC0;
	screenData[2] &= 0xC0;
	screenData[3] &= 0xC0;
	screenData[4] &= 0xC0;
	screenData[5] &= 0xC0;
	
	screenData[0] |= numDataBox[clockObj.getHour()/10];
	screenData[1] |= numDataBox[clockObj.getHour()%10];
	screenData[2] |= numDataBox[clockObj.getMin()/10];
	screenData[3] |= numDataBox[clockObj.getMin()%10];
	screenData[4] |= numDataBox[clockObj.getSec()/10];
	screenData[5] |= numDataBox[clockObj.getSec()%10];
	animationOn = true;
}

void DisplayOut::setDisplaySecs(ClockWorks& clockObj)
{
	
	screenData[0] &= 0xC0;
	screenData[1] &= 0xC0;
	screenData[2] &= 0xC0;
	screenData[3] &= 0xC0;
	screenData[4] &= 0xC0;
	screenData[5] &= 0xC0;
	
	screenData[0] |= numDataBox[clockObj.getMin()/10];
	screenData[1] |= numDataBox[clockObj.getMin()%10];
	screenData[2] |= numDataBox[clockObj.getSec()/10];
	screenData[3] |= numDataBox[clockObj.getSec()%10];
	screenData[4] |= 0;
	screenData[5] |= 0;
	animationOn = true;
	
}
void DisplayOut::setDisplay(uint8_t h, uint8_t m, uint8_t s)
{
	screenData[0] &= 0xC0;
	screenData[1] &= 0xC0;
	screenData[2] &= 0xC0;
	screenData[3] &= 0xC0;
	screenData[4] &= 0xC0;
	screenData[5] &= 0xC0;
	
	screenData[0] |= numDataBox[h/10];
	screenData[1] |= numDataBox[h%10];
	screenData[2] |= numDataBox[m/10];
	screenData[3] |= numDataBox[m%10];
	screenData[4] |= numDataBox[s/10];
	screenData[5] |= numDataBox[s%10];
	animationOn = true;
}
void DisplayOut::setDisplay(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
{
	screenData[0] &= 0xC0;
	screenData[1] &= 0xC0;
	screenData[2] &= 0xC0;
	screenData[3] &= 0xC0;
	screenData[4] &= 0xC0;
	screenData[5] &= 0xC0;
	screenData[0] |= numDataBox[a];
	screenData[1] |= numDataBox[b];
	screenData[2] |= numDataBox[c];
	screenData[3] |= numDataBox[d];
	screenData[4] |= numDataBox[e];
	screenData[5] |= numDataBox[f];
	animationOn = true;
}
void DisplayOut::setDisplay(uint8_t a)
{
	setDisplay(a,a,a,a,a,a);
}
void DisplayOut::setDisplayNoAnimation(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
{
	segDataBuffer[0] = numDataBox[a];
	segDataBuffer[1] = numDataBox[b];
	segDataBuffer[2] = numDataBox[c];
	segDataBuffer[3] = numDataBox[d];
	segDataBuffer[4] = numDataBox[e];
	segDataBuffer[5] = numDataBox[f];
	animationOn = false;
}
void DisplayOut::setDisplayNoAnimation(uint16_t dat)
{
	segDataBuffer[0] = numDataBox[(dat/100000)%10];
	segDataBuffer[1] = numDataBox[(dat/10000)%10];
	segDataBuffer[2] = numDataBox[(dat/1000)%10];
	segDataBuffer[3] = numDataBox[(dat/100)%10];
	segDataBuffer[4] = numDataBox[(dat/10)%10];
	segDataBuffer[5] = numDataBox[dat%10];
	animationOn = false;
}
void DisplayOut::setDisplayNoAnimation(ClockWorks& clockObj)
{
	segDataBuffer[0] = numDataBox[clockObj.getHour()/10];
	segDataBuffer[1] = numDataBox[clockObj.getHour()%10];
	segDataBuffer[2] = numDataBox[clockObj.getMin()/10];
	segDataBuffer[3] = numDataBox[clockObj.getMin()%10];
	segDataBuffer[4] = numDataBox[clockObj.getSec()/10];
	segDataBuffer[5] = numDataBox[clockObj.getSec()%10];
	animationOn = false;
}
void DisplayOut::setDisplayNoAnimationMills(ClockWorks& clockObj)
{
	segDataBuffer[0] = numDataBox[clockObj.getMills()/1000];
	segDataBuffer[1] = numDataBox[(clockObj.getMills()/100)%10];
	segDataBuffer[2] = numDataBox[(clockObj.getMills()/10)%10];
	segDataBuffer[3] = numDataBox[clockObj.getMills()%10];
	segDataBuffer[4] = numDataBox[clockObj.getSec()/10];
	segDataBuffer[5] = numDataBox[clockObj.getSec()%10];
	animationOn = false;
}
void DisplayOut::setDisplayNoAnimationSecs(ClockWorks& clockObj)
{
	segDataBuffer[4] = numDataBox[clockObj.getHour()/10];
	segDataBuffer[5] = numDataBox[clockObj.getHour()%10];
	segDataBuffer[0] = numDataBox[clockObj.getMin()/10];
	segDataBuffer[1] = numDataBox[clockObj.getMin()%10];
	segDataBuffer[2] = numDataBox[clockObj.getSec()/10];
	segDataBuffer[3] = numDataBox[clockObj.getSec()%10];
	animationOn = false;
}
void DisplayOut::setDot(uint8_t digit, bool set, bool right)
{
	if (!set)
	{
		if (right)
		{
			screenData[digit] &= 0xbf;
		}
		else
		{
			screenData[digit] &= 0x7f;
		}
	}
	else
	{
		if (right)
		{
			screenData[digit] |= 0x40;
		}
		else
		{
			screenData[digit] |= 0x80;
		}
		
	}
	animationOn = true;
}
void DisplayOut::setDot(bool set, bool right)
{
	for (int i = 0; i < 6; i++)
	{
		setDot(i, set, right);
	}
	animationOn = true;
}


void DisplayOut::setDisplayBootUp(uint8_t num)
{
	screenData[0] = bootUpDisplayData[num][0];
	screenData[1] = bootUpDisplayData[num][1];
	screenData[2] = bootUpDisplayData[num][2];
	screenData[3] = bootUpDisplayData[num][3];
	screenData[4] = bootUpDisplayData[num][4];
	screenData[5] = bootUpDisplayData[num][5];
	animationOn = true;
}



void DisplayOut::animationTickCounter()
{
	if (animationOn)
	{
		animationTicks[0]++;
		animationTicks[1]++;
		animationTicks[2]++;
		animationTicks[3]++;
		animationTicks[4]++;
		animationTicks[5]++;
	}
}

void DisplayOut::animationRoutin()
{
	if (animationOn)
	{
		for (uint8_t i = 0; i < 6; i++)
		{
			if (screenData[i] & 0x80)
			{
				segDataBuffer[i] |= 0x80;
			}
			else
			{
				segDataBuffer[i] &= 0x7f;
			}
			if (animationTicks[i] > animationRate[i])
			{
				animationTicks[i] = 0;
				if ((segDataBuffer[i]&0x7f) != (screenData[i]&0x7f))
				{
					if (animationCount[i] < 8)
					{
						segDataBuffer[i] &= ~(1<<(animationCount[i]));
					}
					if (animationCount[i] >2)
					{
						segDataBuffer[i] |= (screenData[i]&0x7f)&(1<<(animationCount[i]-3));
					}
					animationCount[i]++;
					if (animationCount[i] == 10)
					{
						animationCount[i] = 0;
					}
				}
				else
				{
					animationCount[i] = 0;
				}
			}
		}
	}
}