/*
    This file is part of GEMS Toolkit.

    GEMS Toolkit is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GEMS Toolkit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with GEMS Toolkit.  If not, see <http://www.gnu.org/licenses/>.
*/ 

#include "midi.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

// get MIDI delta time
std::vector<unsigned char> gett(int t)
{
	std::vector<unsigned char> res;
	if (t <= 0)
	{
		res.push_back(0);
		return res;
	}
	for (int i=0; t>=(1<<(i*7)); ++i)
		res.push_back((i?0x80:0)|((t>>(i*7))&0x7F));
	res[0] &= 0x7F;
	std::reverse(res.begin(),res.end());
	return res;
}

void track_delta(std::vector<unsigned char> &track, int delta)
{
	std::vector<unsigned char> tmp = gett(delta);
	for (int i=0; i<tmp.size(); ++i)
		track.push_back(tmp[i]);
}

void track_text_event(std::vector<unsigned char> &track, int delta, int event, const char * text)
{
	track_delta(track, delta);

	track.push_back(0xFF);  // META
	track.push_back(event); // text event
	track.push_back(strlen(text)); // text length
	for (int i=0; text[i]; ++i)
		track.push_back(text[i]);
}

// add MIDI META track name
void track_name(std::vector<unsigned char> &track, const char * name)
{
	track_text_event(track,0,3,name);
}

// add MIDI META instrument name
void track_instrument_name(std::vector<unsigned char> &track, const char * name)
{
	track_text_event(track,0,4,name);
}

// setup MIDI pitch sensivity
void track_pitch_sens(std::vector<unsigned char> &track, int channel, int semitones)
{
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(100); // CC#100
	track.push_back(0);   // value

	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(101); // CC#101
	track.push_back(0);   // value
	
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(6); // CC#6
	track.push_back(semitones);   // value
	
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(100);  // CC#100
	track.push_back(0x7F); // value
}

// add MIDI META end
void track_end(std::vector<unsigned char> &track, int delta)
{
	track_delta(track, delta);
	track.push_back(0xFF); // META
	track.push_back(0x2F); // end
	track.push_back(0);
}
