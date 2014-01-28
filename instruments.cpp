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

void GemsFM::Set(const BYTE *data)
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

void GemsFM::Write(BYTE *data) const
{
	// Offset 0
	data[0] = type;

	// Offset 1
	data[1] = (unk1<<4)
	         |((LFO_on&1)<<3)
	         |(LFO_val&7);

	// Offset 2
	data[2] = (CH3<<6)
	         |(unk2&0x3F);

	// Offset 3
	data[3] = (unk3<<6)
	         |((FB&7)<<3)
	         |(ALG&7);

	// Offset 4
	data[4] = (L<<7)
	         |((R&1)<<6)
	         |((AMS&3)<<4)
	         |((unk4&1)<<3)
	         |(FMS&7);

	for (int i = 0; i < 4; ++i)
		OP[i].Write(data + 5 + (((i<<1)|(i>>1))&3)*6);

	for (int i = 0; i < 4; ++i)
		SetWordBE(data + 29 + i*2, CH3_F[i]);
	
	data[37] = (unk6<<4)
	          |(KEY&0xF);
	data[38] = unk7;
}

bool GemsFM::IsOn(int op) const
{
	return KEY&(1<<op);
}

bool GemsFM::IsCarrier(int op) const
{
	return FM_ALG_Carriers[ALG]&(1<<op);
}

void GemsFMOperator::Set(const BYTE *data)
{
	// Offset 0
	unk1 = data[0]>>7;
	DT = (data[0]>>4)&7;
	MUL= data[0]&0xF;

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

void GemsFMOperator::Write(BYTE *data) const
{
	// Offset 0
	data[0] = (unk1<<7)
	         |((DT&7)<<4)
	         |(MUL&0xF);

	// Offset 1
	data[1] = (unk2<<7)
	         |(TL&0x7F);

	// Offset 2
	data[2] = (RS<<6)
	         |((unk3&1)<<5)
	         |(AR&0x1F);

	// Offset 3
	data[3] = (AM<<7)
	         |((unk4&3)<<5)
	         |(DR&0x1F);

	// Offset 4
	data[4] = (unk5<<5)
	         |(SDR&0x1F);

	// Offset 5
	data[5] = (SL<<4)
	         |(RR&0xF);
}

void GemsPSG::Set(const BYTE *data)
{
	type = data[0];
	ND = data[1];
	AR = data[2];
	SL = data[3];
	AL = data[4];
	DR = data[5];
	RR = data[6];
};

void GemsPSG::Write(BYTE *data) const
{
	data[0] = type;
	data[1] = ND;
	data[2] = AR;
	data[3] = SL;
	data[4] = AL;
	data[5] = DR;
	data[6] = RR;
};

void InstrumentConverter::Set(const BYTE *data)
{
	int j = 0;
	for (int i=0; i<4; ++i)
	{
		op[i].reg30 = data[j++];
		op[i].reg40 = data[j++];
		op[i].reg50 = data[j++];
		op[i].reg60 = data[j++];
		op[i].reg70 = data[j++];
		op[i].reg80 = data[j++];
		op[i].reg90 = data[j++];
	}
	regB0 = data[j++];
	regB4 = data[j++];
	reg22 = data[j++];
	reg28 = data[j++];
	reg27 = data[j++];
	for (int i=0; i<4; ++i)
	{
		CH3_F[i] = data[j++];
		CH3_F[i] |= (data[j++]<<8);
	}
}

void InstrumentConverter::Write(BYTE *data) const
{
	int j = 0;
	for (int i=0; i<4; ++i)
	{
		data[j++] = op[i].reg30;
		data[j++] = op[i].reg40;
		data[j++] = op[i].reg50;
		data[j++] = op[i].reg60;
		data[j++] = op[i].reg70;
		data[j++] = op[i].reg80;
		data[j++] = op[i].reg90;
	}
	data[j++] = regB0;
	data[j++] = regB4;
	data[j++] = reg22;
	data[j++] = reg28;
	data[j++] = reg27;
	for (int i=0; i<4; ++i)
	{
		data[j++] = CH3_F[i];
		data[j++] = CH3_F[i]>>8;
	}
}

void InstrumentConverter::ImportGems(const BYTE *data)
{
	int j = 0;
	reg22 = data[j++]; // LFO
	reg27 = data[j++]; // CSM
	regB0 = data[j++]; // FB/ALG
	regB4 = data[j++]; // L/R/AMS/FMS
	for (int i=0; i<4; ++i)
	{
		op[i].reg30 = data[j++]; // DT/MUL
		op[i].reg40 = data[j++]; // TL
		op[i].reg50 = data[j++]; // RS/AR
		op[i].reg60 = data[j++]; // AM/DR
		op[i].reg70 = data[j++]; // SR
		op[i].reg80 = data[j++]; // SL/RR
		op[i].reg90 = 0;         // SSG
	}
	for (int i=0; i<4; ++i)
	{
		CH3_F[i] = (data[j++]<<8);
		CH3_F[i] = data[j++];
	}
	reg28 = data[j++]<<4; // Key On
}

void InstrumentConverter::ExportGems(BYTE *data) const
{
	int j = 0;
	data[j++] = reg22; // LFO
	data[j++] = reg27; // CSM
	data[j++] = regB0; // FB/ALG
	data[j++] = regB4; // L/R/AMS/FMS
	for (int i=0; i<4; ++i)
	{
		data[j++] = op[i].reg30; // DT/MUL
		data[j++] = op[i].reg40; // TL
		data[j++] = op[i].reg50; // RS/AR
		data[j++] = op[i].reg60; // AM/DR
		data[j++] = op[i].reg70; // SR
		data[j++] = op[i].reg80; // SL/RR
	}
	for (int i=0; i<4; ++i)
	{
		data[j++] = CH3_F[i]>>8;
		data[j++] = CH3_F[i];
	}
	data[j++] = reg28>>4; // Key On
}

void InstrumentConverter::ImportTYI(const BYTE *data)
{
	for (int i=0; i<4; ++i)
	{
		op[i].reg30 = data[ 0+i]; // DT/MUL
		op[i].reg40 = data[ 4+i]; // TL
		op[i].reg50 = data[ 8+i]; // RS/AR
		op[i].reg60 = data[12+i]; // AM/DR
		op[i].reg70 = data[16+i]; // SR
		op[i].reg80 = data[20+i]; // SL/RR
		op[i].reg90 = data[24+i]; // SSG
	}
	regB0 = data[28]; // FB/ALG
	regB4 = data[29]; // FMS/AMS
	// 30, 31 = YI
	reg22 = 0;    // LFO = Off
	reg28 = 0xF0; // Key On = All
	reg27 = 0;    // CSM = Off
	for (int i=0; i<4; ++i)
		CH3_F[i] = 0;
}

void InstrumentConverter::ExportTYI(BYTE *data) const
{
	int j = 0;
	for (int i=0; i<4; ++i)
	{
		data[ 0+i] = op[i].reg30; // DT/MUL
		data[ 4+i] = op[i].reg40; // TL
		data[ 8+i] = op[i].reg50; // RS/AR
		data[12+i] = op[i].reg60; // AM/DR
		data[16+i] = op[i].reg70; // SR
		data[20+i] = op[i].reg80; // SL/RR
		data[24+i] = op[i].reg90; // SSG
	}
	data[28] = regB0; // FB/ALG
	data[29] = regB4; // FMS/AMS
	data[30] = 'Y';
	data[31] = 'I';
}

void InstrumentConverter::ImportTFI(const BYTE *data)
{
	for (int i=0; i<4; ++i)
	{
		int j = ((i<<1)|(i>>1))&3;
		const BYTE *p = data+2+i*10;
		op[j].reg30 = p[0]&0xF; // MUL;
		int dt = p[1]&7;
		dt -= 3;
		op[j].reg30 |= (dt<0?(-dt)|4:dt)<<4; // DT
		op[j].reg40 = p[2]&0x7F; // TL
		op[j].reg50 = (p[4]&0x1F)|(p[3]<<6); // AR
		op[j].reg60 = p[5]&0x1F;     // DR
		op[j].reg70 = p[6]&0x1F;     // SDR
		op[j].reg80 = (p[8]<<4)|p[7];// SL/RR
		op[j].reg90 = p[9]&0xF;      // SSG
	}
	regB0 = (data[0]&7)|((data[1]&7)<<3); // FB/ALG
	regB4 = 0xC0; // L/R/FMS/AMS
	// 30, 31 = YI
	reg22 = 0;    // LFO = Off
	reg28 = 0xF0; // Key On = All
	reg27 = 0;    // CSM = Off
	for (int i=0; i<4; ++i)
		CH3_F[i] = 0;
}

void InstrumentConverter::ExportTFI(BYTE *data) const
{
	data[0] = regB0&7; // ALG
	data[1] = (regB0>>3)&7; // FB
	for (int i=0; i<4; ++i)
	{
		int j = ((i<<1)|(i>>1))&3;
		BYTE *p = data+2+i*10;
		p[0] = op[j].reg30&0xF; // MUL;
		p[1] = 3+((op[j].reg30>>4)&3)*(op[j].reg30&0x40?-1:1); // DT
		p[2] = op[j].reg40&0x7F; // TL
		p[3] = op[j].reg50>>6;   // RS
		p[4] = op[j].reg50&0x1F; // AR
		p[5] = op[j].reg60&0x1F; // DR
		p[6] = op[j].reg70&0x1F; // SDR
		p[7] = op[j].reg80&0xF;  // RR
		p[8] = op[j].reg80>>4;   // SL
		p[9] = op[j].reg90&0xF;  // SSG
	}
}
