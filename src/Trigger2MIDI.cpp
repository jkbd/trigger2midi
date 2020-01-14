#include "Trigger2MIDI.hpp"

namespace jkbd {
	void Trigger2MIDI::sample_rate(double sr) {
		assert((8000.0 <= sr) and (sr <= 192000.0));
		Trigger2MIDI::sr = sr;
	}
	
	void Trigger2MIDI::run(uint32_t n_samples) {
		forge->prepare(midi_out);

		const float c0 = (M_PI / Trigger2MIDI::sr);
		const float attack = 128.0;
		// No need to smooth user input since we never listen
		// to the side-chain.
		const float release = attack * (*mask_retrigger);	      
		assert(attack < release);

		const float threshold = std::pow(10.0f, (-0.0500000007f * (*dynamic_range)));
		assert((0.0 < threshold) && (threshold < 1.0));

		const float lowpass_freq = 10.0;
		
		float s0 = std::exp(0.0f - (c0 / release));
		float s1 = 1.0f - s0;
		float s2 = std::exp(0.0f - (c0 / attack));
		float s3 = 1.0f - s2;

		float s4 = 1.0f / std::tan(c0 * lowpass_freq);
		float s5 = 1.0f / (s4 + 1.0f);
		float s6 = 1.0f - s4;

		for (uint32_t n = 0; n < n_samples; ++n) {
			x[0] = trigger_in[Trigger2MIDI::Port::SNARE][n];

			// Envelope difference
			r0[0] = std::max<float>(x[0], ((s0 * r0[1]) + (s1 * x[0])));
			r1[0] = (s2 * r1[1]) + (s3 * r0[0]);
			a[0] = std::max<float>(0.0f, (r0[0] - r1[0]));
			
			// Debug envelope output
			cv_out[n] = a[0];
			
			// Analyse onset window
			if (a[0] > threshold) {
				// While the amplitude envelope is
				// high, measure the signal peak.
				peak[0] = (std::fabs(x[0]) > peak[0]) ? std::fabs(x[0]) : peak[0];
			} else {
				// If the amplitude envelope is low
				// but was high before, emit a MIDI
				// Note-On event.
				if (a[1] > threshold) {
					// Send event
					const uint32_t velocity = std::min<int>(127, std::max<int>(0, (int)(127 * peak[0])));
					const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range
					float dB = 20*log10(peak[0]);
					std::cerr << "HIT! " << 127*1.2*peak[0] << ", " << peak[0] << ", " << dB << std::endl;
					int note = static_cast<int>(*note_number);
					forge->enqueue_midi_note(note, velocity, frame_time);

					// Reset the measurement.
					peak[1] = peak[0];
					peak[0] = std::fabs(x[0]);
				} else {
					// Nothing to do
				}
			}
		
			// Lowpass sustain envelope
			b[0] = std::max<float>(0.0f, (r1[0] - r0[0]));			
			v0[0] = b[0];
			r2[0] = 0.0f - (s5 * ((s6 * r2[1]) - (v0[1] + b[0])));
			c[0] = r2[0];

			// Analyse release window
			if (c[0] > threshold) {
			} else {
				if (c[1] > threshold) {
					// Send event
					const uint32_t velocity = 0;
					const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range
					std::cerr << "END! " << std::endl;
					int note = static_cast<int>(*note_number);
					forge->enqueue_midi_note(note, velocity, frame_time);
				} else {
					// Nothing to do
				}
			}
	
			a[1] = a[0];
			b[1] = b[0];			
			x[1] = x[0];

			v0[1] = v0[0];
			
			r0[1] = r0[0];
			r1[1] = r1[0];
			r2[1] = r2[0];
			
		}
		forge->finish();
	}
	
	static LV2_Handle instantiate(const LV2_Descriptor*     descriptor,
				      double                    rate,
				      const char*               bundle_path,
				      const LV2_Feature* const* features)
	{
		Trigger2MIDI* self = new Trigger2MIDI();
		self->sample_rate(rate);

		// Scan the host features
		for (int i=0; features[i]; ++i) {
			if (!strcmp(features[i]->URI, LV2_URID__map)) {
				self->map = static_cast<LV2_URID_Map*>(features[i]->data);
			}
		}
		if (self->map) {
			self->forge = new MIDIEventForge(self->map);		
		} else {
			std::cerr << "Error: Host does not support urid:map" << std::endl;
			delete self;
			return nullptr;
		}
		
		return static_cast<LV2_Handle>(self);
	}
	
	static void connect_port(LV2_Handle instance, uint32_t port, void* data)
	{
		Trigger2MIDI* self = static_cast<Trigger2MIDI*>(instance);
		switch (static_cast<Trigger2MIDI::Port>(port)) {
		case Trigger2MIDI::Port::SNARE:
			self->trigger_in[Trigger2MIDI::Port::SNARE] = static_cast<float*>(data);
			break;
		case Trigger2MIDI::Port::CV_OUT:
			self->cv_out = static_cast<float*>(data);
			break;			
		case Trigger2MIDI::Port::MIDI_OUT:
			self->midi_out = static_cast<LV2_Atom_Sequence*>(data);
			break;
		case Trigger2MIDI::Port::NOTE_NUMBER:
			self->note_number = static_cast<float*>(data);
			break;
		case Trigger2MIDI::Port::MASK_RETRIGGER:
			self->mask_retrigger = static_cast<float*>(data);
			break;
		case Trigger2MIDI::Port::DYNAMIC_RANGE:
			self->dynamic_range = static_cast<float*>(data);
			break;
		case Trigger2MIDI::Port::NOTE_LENGTH:
			self->note_length = static_cast<float*>(data);
			break;			
		}
	}
	
	static void activate(LV2_Handle instance) {
	}
	
	static void run(LV2_Handle instance, uint32_t n_samples) {
		Trigger2MIDI* self = static_cast<Trigger2MIDI*>(instance);
		self->run(n_samples);
	}
	
	static void deactivate(LV2_Handle instance) {
	}
	
	static void cleanup(LV2_Handle instance) {
		// TODO: handle `forge`
		delete static_cast<Trigger2MIDI*>(instance);
	}
	
	static const void* extension_data(const char* uri) {
		return nullptr;
	}  
}

// Force no C++ name mangeling
extern "C" {  
	const LV2_Descriptor* lv2_descriptor(uint32_t index) {
		switch (index) {
		case 0:  return &jkbd::descriptor;
		default: return nullptr;
		}
	}
}
