/*
*	Music.cpp
*
*	Created: 2018-06-26 오전 6:36:52
*	Author: Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose.
*
*/

#include "Music.h"
#define _CTRLA_ON 0b00001011 // 1/64 Prescaler - 312500/TCA_PER Hz, Timer Enabled.
#define _CTRLA_OFF 0b00000110 // Prescaler setting remains.
#define _CTRLB_ON 0b01001011 // Compare 2(WO2) enabled. Auto lock update enabled. Single slope PWM mode.
#define _CTRLB_OFF 0b00001011 // Compare 2 disabled.

void MusicObj::dataRead(uint16_t data)
{
	/*musicTime = (data>>10)&(0b00111111); // Max 63
	uint8_t tone = (data>>3)&(0b01111111);
	uint32_t noteBuffer = (noteBox[tone%12]>>(tone/12));
	uint8_t channelVolume = data&0b00000111;*/
	
	musicTime = (data>>9)&(0b01111111); // Max 63
	uint8_t tone = (data>>3)&(0b00111111);
	uint32_t noteBuffer = (noteBox[tone%12]>>((tone/12)+3));
	uint8_t channelVolume = data&0b00000111;
	
	TCA0_SINGLE_CTRLA = _CTRLA_OFF;
	TCA0_SINGLE_CNT = 0; // Reset timer count register.
	TCA0_SINGLE_PER = noteBuffer; // PER register defines the PWM frequency. 312500/PER Hz.
	TCA0_SINGLE_CMP2 = (noteBuffer*channelVolume)>>4; //CMP2 register defines the duty cycle. CMP2/PER %.
	if (channelVolume == 0)
	{
		musicDown();
	}
	else
	{
		musicOut();
	}
	/*
	//In case of overflow
	if (noteBuffer > 8)
	{
		TCA0_SINGLE_CMP2 = (noteBuffer>>3)*channelVolume;
	}
	else
	{
		TCA0_SINGLE_CMP2 = (noteBuffer*channelVolume)>>3;
	}
	*/
}

void MusicObj::musicDown()
{
	TCA0_SINGLE_CTRLA = _CTRLA_OFF;
	TCA0_SINGLE_CTRLB = _CTRLB_OFF;
	TCA0_SINGLE_CNT = 0;
	TCA0_SINGLE_PER = 0;
	TCA0_SINGLE_CMP2 = 0;
	PORTB_OUTCLR = 0b00000100;
}

void MusicObj::musicOff()
{
	onFlag = false;
	musicDown();
	musicPosition = 0;
}

void MusicObj::musicOut()
{
	TCA0_SINGLE_CTRLA = _CTRLA_ON;
	TCA0_SINGLE_CTRLB = _CTRLB_ON;
}

MusicObj::MusicObj(uint16_t tickFreq) // tickFreq - Ticks to reach 1 second.
{
	//Clear Output on Compare match, Fast PWM, Top Input Capture Register
	TCA0_SINGLE_CTRLA = _CTRLA_OFF;
	TCA0_SINGLE_CTRLB = _CTRLB_OFF;
	TCA0_SINGLE_CTRLC = 0;
	TCA0_SINGLE_CTRLD = 0;
	PORTB_DIRSET = 0b00000100;

	musicPosition = 0;
	musicTime = 0;
	musicTicks = 0;
	
	/* ---Calculation of PER values.---
	* A4 is 440Hz, Frequency is increased 2^(1/12) times every half Note.
	* The PWM outputs 312500/PER Hz, where PER is 16 bits, therefore having range of 4 ~ 312500 Hz. (Roughly C-2 ~ C12)
	* PER value required for the -1th octave (C-1 to B-1) is stored in the array noteBox. (38223, 36077, 34052, 32141, 30337, 28635, 27027, 25511, 24079, 22727, 21452, 20248)
	* The program calculates PER values for other octaves by dividing (or multiplying) the C3 ~ B3 PER values by 2 in dataRead function.
	* MIDI format uses 7 bit numbers to format tones, where 0 is C-1, 127 is  G9, C3 is 48.
	* Therefore, PER = (noteBox[MIDI_TONE%12] >> (MIDI_TONE/12))
	*/
	
	noteBox[0] = 37079;
	noteBox[1] = 34998;
	noteBox[2] = 33033;
	noteBox[3] = 31179;
	noteBox[4] = 29429;
	noteBox[5] = 27778;
	noteBox[6] = 26219;
	noteBox[7] = 24747;
	noteBox[8] = 23358;
	noteBox[9] = 22047;
	noteBox[10] = 20810;
	noteBox[11] = 19642;
	
	musicTimeConst = (tickFreq/12); //80ms

	sheetLength = sizeof(sheet0)/2;

	musicOff();
}

void MusicObj::musicFunction()
{
	if (onFlag)
	{
		if (musicTicks > musicTime*musicTimeConst)
		{
			musicTicks = 0;
			musicPosition++;
			if (musicPosition == sheetLength)
			{
				musicPosition = 0;
			}
			uint16_t buf = pgm_read_word(sheet0+musicPosition);

			dataRead(buf);
		}
		else if ((musicTicks > musicTime*musicTimeConst*3/5)&&(musicTime*musicTimeConst-musicTicks<120))
		{
			if (musicTicks != 0b00111111)
			{
				TCA0_SINGLE_CMP2  = TCA0_SINGLE_CMP2>>2;
			}
		}
	}
}
void MusicObj::autoRoutine()
{
	musicTicks++;
	if(musicTicks & 0x8000)
	{
		musicTicks = 0;
	}
}

