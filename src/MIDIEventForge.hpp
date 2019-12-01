// ISC License
//
// Copyright (c) 2019 Jakob Dübel <jkbd@posteo.de>
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
// OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef MIDI_EVENT_FORGE_H
#define MIDI_EVENT_FORGE_H

#include <cmath>
#define _USE_MATH_DEFINES

#include "lv2.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

// A namespace to force these symbols being not exported in the shared
// library.
namespace jkbd {
	class MIDIEventForge {
	public:
		MIDIEventForge(LV2_URID_Map* map); // , LV2_Atom_Sequence* sink
		//~MIDIEventForge();

		void prepare(LV2_Atom_Sequence* sink);
		void enqueue_midi_note(int8_t velocity, int64_t frame_time);
		void finish();
		
	private:
		LV2_URID midi_MidiEvent;
		LV2_URID atom_Sequence;
		LV2_Atom_Forge forge;
		
		LV2_Atom_Forge_Frame frame;		

		void enqueue(uint8_t raw_midi[3], int64_t frame_time);
		
		// void midi_note(int64_t tme, uint8_t velocity);		
		// bool index{ false };

		//float* target_midi_note;
		//uint8_t midi_note_value{ 42 };

	};	
} // namespace jkbd

#endif // MIDI_EVENT_FORGE_H
