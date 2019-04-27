/*
*	MCP7940N_RTC.cpp
*
*	Created: 2018-07-13 오후 8:40:18
*	Author: Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose.
*
*
*	2019.04.03 - Modified for the Nixie816 Clock, which uses DS1307 as it's RTC.
*	Address included in the code (1101000) is the address of DS1307 RTC chip, not MCP7940N. Address of MCP7940 is 1101111.
*	DS1307 also requires the 7 bit of the sec register to be 0 for the oscillator to run. MCP7940 requires it to be 1 to run.
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "MCP7940N_RTC.h"

void MCP7940n::i2cWrite(uint8_t address, uint8_t* data, uint8_t num)
{
	while(!i2cFinished);
	PORTB_DIRSET = 0b00000011;
	TWI0_MCTRLA = 0b00000001;
	i2cFinished = false;
	TWI0_MSTATUS = 0b11101101;
	TWI0_MADDR = (address&0xfe);
	
	while(!(TWI0_MSTATUS&(1<<5)));
	while(num != 0)
	{
		TWI0_MDATA = *data;
		while(!(TWI0_MSTATUS&(1<<6)));
		num--;
		data++;
	}
	TWI0_MCTRLB = 0b00000011;
	i2cFinished = true;
}

void MCP7940n::i2cRead(uint8_t address, uint8_t* data, uint8_t num)
{
	while(!i2cFinished);
	PORTB_DIRSET = 0b00000011;
	i2cFinished = false;
	TWI0_MCTRLA = 0b00000001;
	TWI0_MSTATUS = 0b11101101;
	TWI0_MADDR = (address&0xfe);
	while(!(TWI0_MSTATUS&(1<<6)));
	TWI0_MDATA = *data;
	while(!(TWI0_MSTATUS&(1<<6)));
	TWI0_MADDR = (address|1);
	while(!(TWI0_MSTATUS&(1<<7)));
	*data = TWI0_MDATA;
	num--;
	data++;
	while(!(TWI0_MSTATUS&(1<<5)));
	while(num != 0)
	{
		TWI0_MCTRLB = 0b00000010;
		while(!(TWI0_MSTATUS&(1<<7)));
		*data = TWI0_MDATA;
		num--;
		data++;
	}
	TWI0_MCTRLB = 0b00000111;
	i2cFinished = true;
}

void MCP7940n::loadTime()
{
	uint8_t temp[] = {RTCSEC, 0, 0};
	i2cRead(ADR_READ, temp, 3);
	secs = bcd2dec(temp[0]&0x7f);
	mins = bcd2dec(temp[1]);
	hours = bcd2dec(temp[2]);
}


void MCP7940n::saveTime()
{
	uint8_t secTemp = ((dec2bcd(secs))&0x7f);
	uint8_t  temp[] = {RTCSEC, secTemp, dec2bcd(mins), dec2bcd(hours)};
	i2cWrite(ADR_WRITE, temp, 4);
}

void MCP7940n::saveTimeSecZero()
{
	uint8_t  temp[] = {RTCSEC, 0, dec2bcd(mins), dec2bcd(hours)};
	i2cWrite(ADR_WRITE, temp, 4);
}


MCP7940n::MCP7940n() : ClockWorks(0,0,0,0)
{
	i2cFinished = true;
	
	TWI0_CTRLA = 0b00000000; // SDA hold time 0.3us
	TWI0_MBAUD = 20; // 400Khz Baud rate.
	TWI0_MCTRLA = 0b00000001; // Read/Write Interrupt disabled, Enable TWI Master.
	TWI0_MSTATUS = 0b11101101;
	
	loadTime();
	enableOscillator();
	enableBattery();
}
