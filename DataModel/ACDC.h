#ifndef _ACDC_H_INCLUDED
#define _ACDC_H_INCLUDED

#include <iostream>

#define NUM_CH 30 //maximum number of channels for one ACDC board
#define NUM_PSEC 5 //maximum number of psec chips on an ACDC board
#define NUM_SAMP 256 //maximum number of samples of one waveform
#define NUM_CH_PER_CHIP 6 //maximum number of channels per psec chips

class ACDC
{
public:
	ACDC(); //constructor
	~ACDC(); //deconstructor

	//----------local return functions
	int getBoardIndex(); //get the current board index from the acdc
	int getNumCh() {int a = NUM_CH; return a;} //returns the number of total channels per acdc
	int getNumPsec() {int a = NUM_PSEC; return a;} //returns the number of psec chips on an acdc
	int getNumSamp() {int a = NUM_SAMP; return a;} //returns the number of samples for on event
	
	//----------local set functions
	void setBoardIndex(int bi); // set the board index for the current acdc

private:
	//----------all neccessary global variables
	int boardIndex; //var: represents the boardindex for the current board

};

#endif
