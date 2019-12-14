#include "Trigger2MIDI.hpp"

namespace jkbd {
	void Trigger2MIDI::sample_rate(double sr) {
		assert((8000.0 <= sr) and (sr <= 192000.0));
		Trigger2MIDI::sr = sr;
	}
	
	void Trigger2MIDI::run(uint32_t n_samples) {
		forge->prepare(midi_out);

		// Highpass, lowpass and amplitude follower
		const float c0 = (M_PI / std::min<float>(192000.0f, std::max<float>(1.0f, Trigger2MIDI::sr)));
		const float lowpass_freq = 360.0f;
		const float highpass_freq = 90.0f;
		const float release = 256.0f;
		const float attack = release - 1.0;

		float s0 = (1.0f / std::tan(c0 * lowpass_freq));
		float s1 = (1.0f / (s0 + 1.0f));
		float s2 = (1.0f - s0);
		float s3 = std::tan(c0 * highpass_freq);
		float s4 = (1.0f / s3);
		float s5 = (s4 + 1.0f);
		float s6 = (0.0f - (1.0f / (s5 * s3)));
		float s7 = (1.0f / s5);
		float s8 = (1.0f - s4);
		float s9 = std::exp(0.0f - (1.0f / attack));
		float s10 = (1.0f - s9);
		float s11 = std::exp(0.0f - (1.0f / release));
		float s12 = (1.0f - s11);

		for (uint32_t n = 0; n < n_samples; ++n) {
			x[0] = trigger_in[Trigger2MIDI::Port::SNARE][n];

			float tmp0 = x[0];
			v0[0] = tmp0;
			r2[0] = (s6 * v0[1]) - (s7 * ((s8 * r2[1]) - (s4 * tmp0)));
			r1[0] = 0.0f - (s1 * ((s2 * r1[1]) - (r2[0] + r2[1])));
			float tmp1 = std::fabs(r1[0]);
			r0[0] = std::max<float>(tmp1, ((s9 * r0[1]) + (s10 * tmp1)));
			r3[0] = (s11 * r3[1]) + (s12 * r0[0]);
			// Trim the amplitude follower output to [0, 1]
			const float gain = 78.0;
			float tmp3 = (r0[0] - r3[0]) * gain;
			a[0] = std::min<float>(1.0f, std::max<float>(0.0f, tmp3));

			cv_out[n] = a[0];

			const float threshold = 0.25f;
			if (a[0] > threshold) {
				// While the amplitude envelope is
				// high, measure the peak.

				peak[0] = (std::fabs(x[0]) > peak[0]) ? std::fabs(x[0]) : peak[0];
			} else {
				// If the amplitude envelope is low
				// but was high before, emit a MIDI
				// event.
				if (a[1] > threshold) {
					// Send event
					const uint32_t velocity = std::min<int>(127, std::max<int>(0, (int)(127 * peak[0])));
					const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range
					float dB = 20*log10(peak[0]);
					std::cerr << "HIT! " << 127*1.2*peak[0] << ", " << peak[0] << ", " << dB << std::endl;
					forge->enqueue_midi_note(42, velocity, frame_time);

					// Reset the measurement.
					peak[1] = peak[0];
					peak[0] = std::fabs(x[0]);					
				} else {
					// Nothing to do
				}				
			}
			
			a[1] = a[0];
			x[1] = x[0];

			v0[1] = v0[0];
			r2[1] = r2[0];
			r1[1] = r1[0];
			r0[1] = r0[0];
			r3[1] = r3[0];
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
		// TODO handle `forge`
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
