#include "common.h"
#include "instruments.h"

BYTE FM_ALG_Carriers[]=
{
	0x8, // 4
	0x8, // 4
	0x8, // 4
	0x8, // 4
	0xA, // 2, 4
	0xE, // 2, 3, 4
	0xE, // 2, 3, 4
	0xF  // 1, 2, 3, 4
};

void GemsFM::Set(BYTE *data)
{
	// Offset 0
	type = data[0];

	// Offset 1
	unk1 = data[1]>>4;
	LFO_on = (data[1]>>3)&1;
	LFO_val = data[1]&7;

	// Offset 2
	CH3 = data[2]>>6;
	unk2 = data[2]&0x3F;

	// Offset 3
	unk3 = data[3]>>6;
	FB = (data[3]>>3)&7;
	ALG = data[3]&7;

	// Offset 4
	L = data[4]>>7;
	R = (data[4]>>6)&1;
	AMS = (data[4]>>4)&3;
	unk4 = (data[4]>>3)&1;
	FMS = data[4]&7;

	for (int i = 0; i < 4; ++i)
		OP[i].Set(data + 5 + ((i<<1|i>>1)&3)*6);

	for (int i = 0; i < 4; ++i)
		CH3_F[i] = GetWordBE(data + 29 + i*2);
	
	unk6 = data[37]>>4;
	KEY = data[37]&0xF;
	unk7 = data[38];
}

bool GemsFM::IsOn(int op)
{
	return KEY&(1<<op);
}

bool GemsFM::IsCarrier(int op)
{
	return FM_ALG_Carriers[ALG]&(1<<op);
}

void GemsFMOperator::Set(BYTE *data)
{
	// Offset 0
	unk1 = data[0]>>7;
	DT = (data[0]>>4)&7;
	MT = data[0]&0xF;

	// Offset 1
	unk2 = data[1]>>7;
	TL = data[1]&0x7F;

	// Offset 2
	RS = data[2]>>6;
	unk3 = (data[2]>>5)&1;
	AR = data[2]&0x1F;

	// Offset 3
	AM = data[3]>>7;
	unk4 = (data[3]>>5)&3;
	DR = data[3]&0x1F;

	// Offset 4
	unk5 = data[4]>>5;
	SDR = data[4]&0x1F;

	// Offset 5
	SL = data[5]>>4;
	RR = data[5]&0xF;
}
