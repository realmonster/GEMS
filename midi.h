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

#ifndef MIDI_H
#define MIDI_H
#include <vector>

void track_delta(std::vector<unsigned char> &track, int delta);
void track_text_event(std::vector<unsigned char> &track, int delta, int event, const char * text);
void track_name(std::vector<unsigned char> &track, const char * name);
void track_instrument_name(std::vector<unsigned char> &track, const char * name);
void track_pitch_sens(std::vector<unsigned char> &track, int channel, int semitones);
void track_end(std::vector<unsigned char> &track, int delta);

#endif