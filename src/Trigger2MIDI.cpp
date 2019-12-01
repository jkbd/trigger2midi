#include "Trigger2MIDI.hpp"

namespace jkbd {
	// Trigger2MIDI::Trigger2MIDI() {
	// 	forge = new MIDIEventForge();
	// }

	// Trigger2MIDI::~Trigger2MIDI() {}

	
	inline float decibel_to_coef(float gain) {
		return ((gain) > -90.0f ? powf(10.0f, (gain) * 0.05f) : 0.0f);
	}
	
	void Trigger2MIDI::sample_rate(double sr) {
		assert((8000.0 <= sr) and (sr <= 192000.0));
		Trigger2MIDI::sr = sr;
	}
	
	void Trigger2MIDI::run(uint32_t n_samples) {
		forge->prepare(midi_out);
		
		for (uint32_t n = 0; n < n_samples; ++n) {		
			x[0] = trigger_in[Trigger2MIDI::Port::SNARE][n];
			
			bool zero_crossing = (x[0] > 0.0f) && (x[1] <= 0.0f) ||
				(x[0] < 0.0f) && (x[1] >= 0.0f);
			
			if (zero_crossing) {
				//std::cerr << "Zero cross" << std::endl;
				

				// We compare the areas between zero
				// crossings. A peak area is framed by
				// two areas smaller than the peak.
				if (((sum[0]/zero[0]) < (sum[1]/zero[1])) &&
				    ((sum[1]/zero[1]) > (sum[2]/zero[2]))) {
					// The last integral was a
					// peak. If it is above the
					// threshold, send a MIDI
					// event.
					const float integral = sum[1]/zero[1];

					const float threshold = 0.5f; // TODO Adjust
					if ((integral != std::numeric_limits<float>::infinity()) &&
					    (integral > threshold)) {
						std::cerr << "Integral: " << integral << std::endl;
						
						// Send
						const uint32_t velocity = (int) (integral*127); // TODO: scale
						const int64_t frame_time = n-(zero[0]+zero[1]); // TODO guard range
						if (velocity > 0) {
							forge->enqueue_midi_note(42, velocity, frame_time);
						} else {
							// Drop it
						}
					} else {
						// Nothing
					}					
				} else {
					// Just go on
				}

				// Reset the zero crossing counter
				zero[2] = zero[1];
				zero[1] = zero[0];
				zero[0] = 0.0;

				sum[2] = sum[1];				
				sum[1] = sum[0];
				sum[0] = 0.0;
			} else {
				zero[0] += 1;
				sum[0] += std::fabs(x[0]);
			}

			x[1] = x[0];

			// const float abs2 = std::fabs(trigger_in[Trigger2MIDI::Port::SNARE][n]);
			// const bool trigger2 = (abs2 > threshold);		
			// if (trigger2) {
                        //         const uint32_t velocity = (int) (abs2*127);
			// 	forge->enqueue_midi_note(43, velocity, n);
			// }
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
		case Trigger2MIDI::Port::TOM:
			self->trigger_in[Trigger2MIDI::Port::TOM] = static_cast<float*>(data);
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
