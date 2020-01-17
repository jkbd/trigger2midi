// ISC License
//
// Copyright (c) 2019-2020 Jakob Dübel <jkbd@posteo.de>
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

#ifndef TRIGGER2MIDI_H
#define TRIGGER2MIDI_H

#include <iostream>
#include <cmath>
#define _USE_MATH_DEFINES
#include <limits>

#include "lv2.h"
#include "MIDIEventForge.hpp"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

// A namespace to force these symbols being not exported in the shared
// library.
namespace jkbd {
	typedef struct {
		float r0[2]{ 0.0f, 0.0f };
		float r1[2]{ 0.0f, 0.0f };
	} transient_envelope_state_t;

	typedef struct {
		float x[2]{ 0.0f, 0.0f };
		float e[2]{ 0.0f, 0.0f };
		float peak[2]{ 0.0f, 0.0f };
	} onset_noteon_state_t;
	
	class Trigger2MIDI {
	public:
		enum Port {
			SNARE = 0, // must be a valid index in `trigger_in[]`!
			CV_OUT = 1,
			MIDI_OUT = 2,
			NOTE_NUMBER = 3,
			MASK_RETRIGGER = 4,
			DYNAMIC_RANGE = 5
		};
		
		// Port buffers
		float* trigger_in[2];
		LV2_Atom_Sequence* midi_out;
		float* cv_out;
		float* note_number;
		float* mask_retrigger;
		float* dynamic_range;

		transient_envelope_state_t* transient_envelope_state;
		onset_noteon_state_t* onset_noteon_state;
		
		void sample_rate(double sr);
		void run(uint32_t n_samples);

		LV2_URID_Map* map;
		MIDIEventForge* forge;
		
	private:		
		double sr{ 8000.0 };		
	};
	
	static LV2_Handle
	instantiate(const LV2_Descriptor*     descriptor,
		    double                    rate,
		    const char*               bundle_path,
		    const LV2_Feature* const* features);
	
	static void
	connect_port(LV2_Handle instance,
		     uint32_t   port,
		     void*      data);
	
	static void
	activate(LV2_Handle instance);
	
	static void
	run(LV2_Handle instance, uint32_t n_samples);
	
	static void
	deactivate(LV2_Handle instance);
	
	static void
	cleanup(LV2_Handle instance);
	
	static const void*
	extension_data(const char* uri);
	
	static constexpr char uri[] = "https://github.com/jkbd/trigger2midi";
	
	static const LV2_Descriptor
	descriptor = {
		jkbd::uri,
		instantiate,
		connect_port,
		activate,
		run,
		deactivate,
		cleanup,
		extension_data
	};
} // namespace jkbd

// Force no C++ name mangeling
extern "C" {  
	const LV2_Descriptor* lv2_descriptor(uint32_t index);
}

#endif // TRIGGER2MIDI_H
