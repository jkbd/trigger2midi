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

#ifndef TRIGGER2MIDI_H
#define TRIGGER2MIDI_H

#include <cmath>
#define _USE_MATH_DEFINES

#include "lv2.h"

// A namespace to force these symbols being not exported in the shared
// library.
namespace jkbd {
	class Trigger2MIDI {
	public:
		enum Port {
			MIDI_OUT = 0,
			SNARE = 1,
		};
		
		// Port buffers
		float*  midi_out;
		float*  snare;		
		
		void sample_rate(double sr);
		void run(uint32_t n_samples);
		
	private:
		// // Note: x[0] is x_{n} and x[1] is backward in time x_{n-1}
		// float f[2]{ 0.0f, 0.0f };
		// float x[2]{ 0.0f, 0.0f };
		// float y[2]{ 0.0f, 1.0f };
		
		// bool index{ false };
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
		instantiate, // instantiate
		connect_port, // connect_port,
		activate, // activate
		run, // run
		deactivate, // deactivate
		cleanup, // cleanup
		extension_data // extension_data	      
	};
} // namespace jkbd

// Force no C++ name mangeling
extern "C" {  
	const LV2_Descriptor* lv2_descriptor(uint32_t index);
}

#endif // TRIGGER2MIDI_H
