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
	// Offset 0  (30H raw register)
	BYTE unk1;  // : 1;
	BYTE DT  ;  // : 3; // Detune
	BYTE MUL ;  // : 4; // Multiply

	// Offset 1 (40H multiplied with volume)
	BYTE unk2;  // : 1;
	BYTE TL  ;  // : 7; // Total Level

	// Offset 2 (50H raw register)
	BYTE RS  ;  // : 2; // Rate Scale
	BYTE unk3;  // : 1;
	BYTE AR  ;  // : 5; // Attack Rate

	// Offset 3 (60H raw register)
	BYTE AM  ;  // : 1; // Amplitude Modulation
	BYTE unk4;  // : 2;
	BYTE DR  ;  // : 5; // Decay Rate

	// Offset 4 (70H raw register)
	BYTE unk5;  // : 3;
	BYTE SDR ;  // : 5; // Sustain Decay Rate

	// Offset 5 (80H raw register)
	BYTE SL  ;  // : 4; // Sustain Level
	BYTE RR  ;  // : 4; // Release Rate

	void Set(const BYTE *data);
	void Write(BYTE *data) const;
};

struct GemsFM : GemsInstrument
{
	// Offset 1:
	BYTE unk1   ; // : 4;
	BYTE LFO_on ; // : 1; // Low Frequency Oscilator on
	BYTE LFO_val; // : 3; // Low Frequency Oscilator value

	// Offset 2:
	BYTE CH3    ; // : 2; // Channel 3 mode
	BYTE unk2   ; // : 6;

	// Offset 3: (B0H raw register)
	BYTE unk3   ; // : 2;
	BYTE FB     ; // : 3; // Feedback
	BYTE ALG    ; // : 3; // Algorithm

	// Offset 4: (B4H raw register)
	BYTE L      ; // : 1; // Left channel on
	BYTE R      ; // : 1; // Right channel on
	BYTE AMS    ; // : 2; // AMS
	BYTE unk4   ; // : 1;
	BYTE FMS    ; // : 3; // FMS

	// Offset 5: operators in order: 1,3,2,4
	GemsFMOperator OP[4]; // here in 1,2,3,4 order
	short CH3_F[4]; // Channel 3 Frequency in 1,2,3,4 order
	BYTE unk6   ; // : 4;
	BYTE KEY    ; // : 4; // Operator On[4]
	BYTE unk7   ; // : 8;
	BYTE unk8[0];

	void Set(const BYTE *data);
	void Write(BYTE *data) const;
	
	// returns KEY on
	bool IsOn(int op) const;
	
	// returns true if Volume depends Operator TL.
	bool GemsFM::IsCarrier(int op) const;
};

struct GemsPSG : GemsInstrument
{
	BYTE ND; // ?????xxx Noise Data
	BYTE AR; // xxxxxxxx Attack Rate
	BYTE SL; // 0000xxxx Sustain Level
	BYTE AL; // 0000xxxx Attack Level
	BYTE DR; // xxxxxxxx Decay Rate
	BYTE RR; // xxxxxxxx Release Rate

	void Set(const BYTE *data);
	void Write(BYTE *data) const;
};

struct RawFMOperator
{
	BYTE reg30; // DT/MUL
	BYTE reg40; // TL
	BYTE reg50; // RS/AR
	BYTE reg60; // AM/DR
	BYTE reg70; // SDR
	BYTE reg80; // SL/RR
	BYTE reg90; // SSG
};

struct InstrumentConverter
{
	RawFMOperator op[4]; // in 1,2,3,4 order
	BYTE regB0; // FB/ALG
	BYTE regB4; // L/R/AMS/FMS
	BYTE reg22; // LFO bits
	BYTE reg28; // Key On bits
	BYTE reg27; // Channel 3 bits
	short CH3_F[4]; // Channel 3 Frequency in 1,2,3,4 order
	void Set(const BYTE *data);
	void Write(BYTE *data) const;

	// size 39
	void ImportGems(const BYTE *data);
	void ExportGems(BYTE *data) const;

	// size 32
	void ImportTYI(const BYTE *data);
	void ExportTYI(BYTE *data) const;

	// size 42
	void ImportTFI(const BYTE *data);
	void ExportTFI(BYTE *data) const;

	// size 29
	void ImportEIF(const BYTE *data);
	void ExportEIF(BYTE *data) const;

	// size 128
	void ImportY12(const BYTE *data);
	void ExportY12(BYTE *data) const;

	// size 43
	void ImportVGI(const BYTE *data);
	void ExportVGI(BYTE *data) const;

	// size 51
	void ImportDMP(const BYTE *data);
	void ExportDMP(BYTE *data) const;
};

#endif

