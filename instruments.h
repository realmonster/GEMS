#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include "common.h"

#define GEMSI_FM    0
#define GEMSI_DAC   1
#define GEMSI_PSG   2
#define GEMSI_NOISE 3

struct GemsInstrument
{
	BYTE type;
};

struct GemsFMOperator
{
	// Offset 0
	BYTE unk1;  // : 1;
	BYTE DT  ;  // : 3; // Detune
	BYTE MUL ;  // : 4; // Multiply

	// Offset 1
	BYTE unk2;  // : 1;
	BYTE TL  ;  // : 7; // Total Level

	// Offset 2
	BYTE RS  ;  // : 2; // Rate Scale
	BYTE unk3;  // : 1;
	BYTE AR  ;  // : 5; // Attack Rate

	// Offset 3
	BYTE AM  ;  // : 1; // Amplitude Modulation
	BYTE unk4;  // : 2;
	BYTE DR  ;  // : 5; // Decay Rate

	// Offset 4
	BYTE unk5;  // : 3;
	BYTE SDR ;  // : 5; // Sustain Decay Rate

	// Offset 5
	BYTE SL  ;  // : 4; // Sustain Level
	BYTE RR  ;  // : 4; // Release Rate

	void Set(const BYTE *data);
	void Write(BYTE *data);
};

struct GemsFM : GemsInstrument
{
	// Offset 1:
	BYTE unk1   ; // : 4;
	BYTE LFO_on ; // : 1; // Low Frequency Oscilator on
	BYTE LFO_val; // : 3; // Low Frequency Oscilator value

	// Offset 2:
	BYTE CH3    ; // : 1; // Channel 3 mode
	BYTE unk2   ; // : 6;

	// Offset 3:
	BYTE unk3   ; // : 2;
	BYTE FB     ; // : 3; // Feedback
	BYTE ALG    ; // : 3; // Algorithm

	// Offset 4:
	BYTE L      ; // : 1; // Left channel on
	BYTE R      ; // : 1; // Right channel on
	BYTE AMS    ; // : 2; // AMS
	BYTE unk4   ; // : 1;
	BYTE FMS    ; // : 3; // FMS

	// Offset 5;
	GemsFMOperator OP[4];
	short CH3_F[4]; //  Channel 3 Frequency
	BYTE unk6   ; // : 4;
	BYTE KEY    ; // : 4; // Operator On[4]
	BYTE unk7   ; // : 8;
	BYTE unk8[0];

	void Set(const BYTE *data);
	void Write(BYTE *data);
	
	// returns KEY on
	bool IsOn(int op);
	
	// returns true if Volume depends Operator TL.
	bool GemsFM::IsCarrier(int op);
};

#endif

