#include <cstdio>
#include <ctype.h>
#include <direct.h>
#include "instruments.h"
#include "vgm_parser.h"
#include "midi.h"

void usage()
{
	printf(
"Usage: vgmjuice input output_dir [options]\n"
"\n"
"      input - vgm file path (NOT *.vgz)\n"
"      output_dir - output directory\n"
"\n"
"Options:\n"
"  -p - (patch) instruments format:\n"
"        gems - internal GEMS 2.8 format (default)\n"
"        tyi  - Tiido's instrument file\n"
"        tfi  - TFM Music Maker instrument file\n"
"        eif  - ECHO instrument file\n"
"        y12  - GensKMOD YM2612 channel dump\n"
"        vgi  - VGM Music Maker instrument file\n" 
"        dmp  - DefleMask instrument file\n" 
"        dmp0 - DefleMask instrument file version 0\n" 
"        smps - internal SMPS format (Sonic 3)\n"
"\n" 
"  -pext - instruments extension (default: format name)\n"
"\n"
"      Warning: all outputs will be overwrited automatically.\n"
"\n"
"Author: r57shell@uralweb.ru\n");
}

int waitTime[2] = { 735, 882 };
int time = 0;
int dacPos = 0;
bool dacOn = 0;
InstrumentConverter channel[6];
int freq[6];
int CH3_F[3]; // Channel 3 Frequences in order A8,A9,AA

struct note
{
	int time, on, freq, patch;
	note() {}
	note(int _time, int _on, int _freq, int _patch) : time(_time), on(_on), freq(_freq), patch(_patch) {}
};
std::vector<note> tracks[6];
int lastInstrument[6];

std::vector<InstrumentConverter> instruments;

int getInstrumentId(InstrumentConverter inst)
{
	int j;
	for (int i=0; i<instruments.size(); ++i)
	{
		InstrumentConverter &x = instruments[i];
		if (inst.regB0 != x.regB0 // FB/ALG
		 || inst.regB4 != x.regB4 // L/R/AMS/FMS
		 || inst.reg28 != x.reg28 // Key On bits
		 || inst.reg27 != x.reg27)// Channel 3 mode bits
			continue;
		if (x.reg27) // if Channel 3 mode is on
		{
			for (j=0; j<4; ++j)
				if (inst.CH3_F[j] != x.CH3_F[j])
					break;
			if (j != 4)
				continue;
		}
		for (j=0; j<4; ++j)
		{
			if (!(inst.reg28&(1<<(i+4)))) // operator is off
				continue;
			if (inst.op[j].reg30 != x.op[j].reg30 // DT/MUL 
			 || inst.op[j].reg50 != x.op[j].reg50 // RS/AR
			 || inst.op[j].reg60 != x.op[j].reg60 // AM/DR
			 || inst.op[j].reg70 != x.op[j].reg70 // SDR
			 || inst.op[j].reg80 != x.op[j].reg80 // SL/RR
			 || inst.op[j].reg90 != x.op[j].reg90)// SSG
				break;
		}
		if (j != 4)
			continue;

		// found same, calculate volume
		int volume_this = 0, volume_new = 0;
		for (j=0; j<4; ++j)
		{
			if (!(inst.reg28&(1<<(i+4)))) // operator is off
				continue;
			volume_this += 0x7F-((x.op[j].reg40)&0x7F);
			volume_new += 0x7F-((inst.op[j].reg40)&0x7F);
		}

		// copy louder
		if (volume_new > volume_this)
			for (j=0; j<4; ++j)
				x.op[j].reg40 = inst.op[j].reg40;
		return i;
	}
	instruments.push_back(inst);
	return instruments.size()-1;
}

static int fmtbl[] =
{
 644, //C
 682, //C#
 723, //D
 766, //D#
 811, //E
 859, //F
 910, //F#
 965, //G
1022, //G#
1083, //A
1147, //A#
1215, //B
1288, //C
};

char *note_name[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

int getNote(int freq)
{
	int f = (freq&0x7FF)<<((freq&0x3800)>>11);
	int o, n = 0;
	for (o=0; o<20; ++o)
		if ((fmtbl[n]<<o) > f)
			break;
	--o;
	for (n=0; n<12; ++n)
		if ((fmtbl[n]<<o) > f)
			break;
	--n;
	if (n<0)
	{
		--o;
		n = 11;
	}
	if (o>=0)
	{
		// log(f)-(note[n]) > log(note[n+1])-log(f)
		if (((long long)f)*f > (long long)(fmtbl[n+1]<<o)*(fmtbl[n]<<o))
			++n;
	}
	if (n == 12)
	{
		++o;
		n = 0;
	}
	n += o*12;
	if (n<0)
		n = 0;
	if (n>=12*8)
		n = 12*8-1;
	return n;
}

double getPitch(int freq, int note)
{
	int f = fmtbl[note%12]<<(note/12);
	int fp = (freq&0x7FF)<<((freq&0x3800)>>11);
	return log(double(fp)/f)/log(2.0);
}

void ym2612write(int port, int reg, int val)
{
	int ch = port*3+(reg&3);
	int s = ((reg>>1)&2)|((reg>>3)&1); // slot
	RawFMOperator *op = 0;

	if (reg>=0x30)
	{
		if ((reg&3) == 3)
		{
			printf("Weird YM2612 reg write: %02X %02X\n",reg,val);
			return;
		}
		else
			op = &(channel[ch].op[s]);

		// Don't spam
		/*if (channel[ch].reg28 // Key On
		&& (reg < 0xA0 || reg > 0xA6))
			printf("Unexpected YM2612 channel change during key on: %02X %02X\n",reg,val);*/
	}

	switch (reg&0xF0)
	{
		case 0x20:
			switch(reg)
			{
				case 0x22: // LFO bits
					for (int i=0; i<6; ++i)
						channel[i].reg22 = val&0xF;
					break;
				case 0x27: // Channel 3 Mode bits
					channel[2].reg27 = val&0xC0;
					break;
				case 0x28: // Key On/Off
					ch = val&7;
					if ((ch&3) == 3)
					{
						printf("Weird YM2612 reg write: %02X %02X\n",reg,val);
						break;
					}

					if (ch > 3)
						--ch; 

					if ((channel[ch].reg28)&&(val&0xF0)&& (val&0xF0) != channel[ch].reg28)
					{
						printf("Unexpected behavior: key on update %02X %02X\n",reg,val);
					}
					if ((channel[ch].reg28==0) != ((val&0xF0)==0))
					{
						// Key On was changed
						channel[ch].reg28 = (val&0xF0);
						int patch = lastInstrument[ch];
						if (val&0xF0) // Key On
						{
							patch = lastInstrument[ch] = getInstrumentId(channel[ch]);
							tracks[ch].push_back(note(time,1,freq[ch],patch));
						}
						else
						{
							tracks[ch].push_back(note(time,0,freq[ch],patch));
						}
					}
					// Key On
					channel[ch].reg28 = (val&0xF0);
					break;
				case 0x2B: // DAC On/Off
					dacOn = (val&0x80)?true:false;
					break;
			}
			break;
		case 0x30: // DT/MUL
			op->reg30 = val;
			break;
		case 0x40: // TL
			op->reg40 = val;
			break;
		case 0x50: // RS/AR
			op->reg50 = val;
			break;
		case 0x60: // AM/DR
			op->reg60 = val;
			break;
		case 0x70: // SDR
			op->reg70 = val;
			break;
		case 0x80: // SL/RR
			op->reg80 = val;
			break;
		case 0x90: // SSG
			op->reg90 = val;
			break;
		case 0xA0: // Frequency
			switch(reg&0xC)
			{
				case 0: // A0-A2 Low
					freq[ch]=(freq[ch]&0xFF00)|val;
					if (channel[ch].reg28) // Key On
						tracks[ch].push_back(note(time,1,freq[ch],lastInstrument[ch]));
					break;
				case 4: // A4-A6 High
					freq[ch]=(freq[ch]&0xFF)|(val<<8);
					break;
				case 8:
					CH3_F[reg&3] = (CH3_F[reg&3]&0xFF00)|val;
					break;
				case 12:
					CH3_F[reg&3] = (CH3_F[reg&3]&0xFF)|(val<<8);
					break;
			}
			// always full update CH3 just to avoid bugs
			channel[2].CH3_F[0] = CH3_F[1]; // A9
			channel[2].CH3_F[1] = CH3_F[0]; // A8
			channel[2].CH3_F[2] = CH3_F[2]; // AA
			channel[2].CH3_F[3] = freq[2];  // A2
			break;
		case 0xB0:
			switch(reg&0xC)
			{
				case 0: // FB/ALG
					channel[ch].regB0 = val;
					break;
				case 4: // L/R/AMS/FMS 
					channel[ch].regB4 = val;
					break;
			}
			break;
		default:
			printf("YM2612_%d %02 %02\n",port,reg,val);
			break;
	}
}

void sn76489write(int value)
{
	// TODO
}

bool process(vgmParser *parser, void *, int cmd, const unsigned char *args)
{
	switch(cmd)
	{
	case 0x4F: // Game Gear PSG stereo, write dd to port 0x06
		// just ignore that
		break;

	case 0x50: // PSG (SN76489/SN76496) write value dd
		sn76489write(args[0]);
		break;

	case 0x52: // YM2612 port 0
	case 0x53: // YM2612 port 1
		ym2612write(cmd&1,args[0],args[1]);
		break;

	case 0x61: // Wait
		time += GetWordLE(args);
		break;

	case 0x62: // Wait NTSC
	case 0x63: // Wait PAL
		time += waitTime[cmd-0x62];
		break;

	case 0x64: // Wait Override
		if (args[0]==0x62
		 || args[0]==0x63)
			waitTime[args[0]-0x62] = GetWordLE(args+1);
		break;

	case 0x66: // End of Sound data
		break;

	case 0xE0: // PCM Seek
		dacPos = GetLongLE(args);
		break;

	default:
		if ((cmd&0xF0)==0x70) // Wait n+1
		{
			time += (cmd&0xF)+1;
		}
		else if ((cmd&0xF0)==0x80) // YM2612 dac port 0
		{
			time += (cmd&0xF);
			++dacPos;
		}
		else
		{
			printf("Unhandled: %X\n",cmd);
		}
	}
	return false;
}

char *pext = 0;
int instrument_format = 0;

char *formats[] = {
	"gems",
	"tyi",
	"tfi",
	"eif",
	"y12",
	"vgi",
	"dmp",
	"dmp0",
	"smps",
};

int format_size[] = {
	39,  // GEMS
	32,  // TYI
	42,  // TFI
	29,  // EIF
	128, // Y12
	43,  // VGI
	51,  // DMP
	49,  // DMP v0
	25,  // SMPS
};

int getformat(const char *name)
{
	for (int i=0; i<sizeof(format_size)/sizeof(format_size[0]); ++i)
	{
		bool same = true;
		int j;
		for (j=0; name[j] && formats[i][j]; ++j)
			if (tolower(name[j]) != tolower(formats[i][j]))
				same = false;
		if (same && !name[j] && !(formats[i][j]))
			return i;
	}
	return -1;
}

int main(int argc, char **args)
{
	if (argc < 3)
	{
		usage();
		return 0;
	}
	for (int i=3; i<argc;)
	{
		if (!strcmp(args[i],"-p"))
		{
			if (i+1<argc)
			{
				instrument_format = getformat(args[i+1]);
				if (instrument_format < 0)
				{
					printf("Error: unknown instrument type \"%s\"\n\n",args[i+1]);
					usage();
					return 0;
				}
			}
			else
			{
				printf("Error: instrument type expected in command line\n\n");
				usage();
				return 0;
			}
			i+=2;
		}
		else if (!strcmp(args[i],"-pext"))
		{
			if (i+1<argc)
				pext = args[i+1];
			else
			{
				printf("Error: extension expected in command line\n\n");
				usage();
				return 0;
			}
			i+=2;
		}
		else
		{
			printf("Error: unknown command line option \"%s\"\n\n",args[i]);
			usage();
			return 0;
		}
	}
	
	if (!pext)
		pext = formats[instrument_format];

	FILE *vgm = fopen(args[1],"rb");
	if (!vgm)
	{
		printf("Error: Can't open file \"%s\"\n\n",args[1]);
		usage();
		return 1;
	}

	unsigned char buff[200];

	vgmParser parser;
	parser.process = process;
	int ret = parser.readHeader(vgm);
	if (ret)
	{
		printf("Error: something with header :)\n");
	}

	printf("Version: %X\nPSG Clock: %d\nYM2612 Clock: %d\n",
		parser.getVersion(),
		parser.clockSN76489(),
		parser.clockYM2612());

	ret = parser.parse(vgm);
	fclose(vgm);
	if (ret == 1)
	{
		printf("Error: unexpected end of vgm\n");
		return 2;
	}
	if (ret == 2)
	{
		printf("Error: invalid command\n");
	}

	mkdir(args[2]);
	sprintf((char*)buff,"%s\\vgm.mid",args[2]);

	FILE *file = fopen((char*)buff,"wb");
	if (!file)
	{
		printf("Error: Can't create file \"%s\"\n",buff);
		return 2;
	}

	// MIDI header
	strcpy((char*)buff,"MThd");
	buff[4]=buff[5]=buff[6]=0;
	buff[7]=6;
	buff[8]=buff[10]=0;
	buff[9]=1; // format multiple sync tracks
	buff[11]=1+6; // tracks count
	int ppq = 44100/2; // (Pulses per quater)
	buff[12]=ppq>>8;
	buff[13]=ppq;

	// write MIDI header
	fwrite(buff,1,14,file);

	// add MIDI META tempo
	int tempo = 120;
	int microseconds = (int)(double(60*1000*1000)/tempo);
	buff[0]=0;
	buff[1]=0xFF; // meta
	buff[2]=0x51; // set tempo
	buff[3]=3;
	buff[4]=microseconds>>16;
	buff[5]=microseconds>>8;
	buff[6]=microseconds;
	std::vector<unsigned char> track;
	for (int tmp1=0; tmp1<7; ++tmp1)
		track.push_back(buff[tmp1]);
	track_end(track,0);

	// Write MIDI track.
	strcpy((char*)buff,"MTrk");
	buff[4] = track.size()>>24;
	buff[5] = track.size()>>16;
	buff[6] = track.size()>>8;
	buff[7] = track.size();
	fwrite(buff,1,8,file);
	fwrite(&track[0],1,track.size(),file);

	for (int i=0; i<6; ++i)
	{
		int lastPitch = 0x2000;
		int lastNote = 0;
		int lastTime = 0;
		int lastOn = 0;

		track.clear();
		// MIDI init
		sprintf((char*)buff,"Channel %02X",i);
		track_name(track,(char*)buff);
		track_instrument_name(track,(char*)buff);
		track_pitch_sens(track, i, 12);

		for (int j=0; j<tracks[i].size(); ++j)
		{
			note &n = tracks[i][j];
			if (n.on)
			{
				if (!lastOn)
				{
					track_delta(track, n.time-lastTime);
					lastNote = getNote(n.freq);

					// set pitch before key on
					double pitch = getPitch(n.freq, lastNote);
					int pv = 0x2000+pitch*0x2000;
					if (pv>=0x4000)
						pv=0x3FFF;
					if (pv<0)
						pv=0; 
					if (pv != lastPitch)
					{
						track.push_back(0xE0|i);
						track.push_back(pv&0x7F);
						track.push_back((pv>>7)&0x7F); 
						track_delta(track, 0);
					}
					track.push_back(0x90|i); // key on
					track.push_back(lastNote);
					track.push_back(0x7F); // volume
					lastTime = n.time;
					lastOn = 1;
					lastPitch = pv;
				}
				else
				{
					double pitch = getPitch(n.freq, lastNote);
					int pv = 0x2000+pitch*0x2000;
					if (pv>=0x4000)
						pv=0x3FFF;
					if (pv<0)
						pv=0; 

					if (pv != lastPitch)
					{
						track_delta(track, n.time-lastTime);
						track.push_back(0xE0|i);
						track.push_back(pv&0x7F);
						track.push_back((pv>>7)&0x7F);
						lastTime = n.time;
						lastPitch = pv;
					}
				}
			}
			else
			{
				track_delta(track, n.time-lastTime);
				track.push_back(0x80|i); // key off
				track.push_back(lastNote);
				track.push_back(0x7F); // volume
				lastTime = n.time;
				lastOn = 0;
			}
		}
		track_end(track,0);

		// Write MIDI track.
		strcpy((char*)buff,"MTrk");
		buff[4] = track.size()>>24;
		buff[5] = track.size()>>16;
		buff[6] = track.size()>>8;
		buff[7] = track.size();
		fwrite(buff,1,8,file);
		fwrite(&track[0],1,track.size(),file);
	}

	fclose(file);

	for (int i=0; i<instruments.size(); ++i)
	{
		sprintf((char*)buff,"%s\\patch_%02d.%s",args[2],i,pext);

		InstrumentConverter &ic = instruments[i];

		file = fopen((char*)buff,"wb");
		if (!file)
		{
			printf("Error: Can't create file \"%s\"\n",buff);
			continue;
		}
		switch(instrument_format)
		{
			case 0: // GEMS
				ic.ExportGems(buff);
				break;
			case 1: // TYI
				ic.ExportTYI(buff);
				break;
			case 2: // TFI
				ic.ExportTFI(buff);
				break;
			case 3: // EIF
				ic.ExportEIF(buff);
				break;
			case 4: // Y12
				ic.ExportY12(buff);
				break;
			case 5: // VGI
				ic.ExportVGI(buff);
				break;
			case 6: // DMP
				ic.ExportDMP(buff);
				break;
			case 7: // DMPv0
				ic.ExportDMPv0(buff);
				break;
			case 8: // SMPS
				ic.ExportSMPS(buff);
				break;
		}
		fwrite(buff,1,format_size[instrument_format],file);
		fclose(file);
	}

	return 0;
}
