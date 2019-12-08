#include "Trigger2MIDI.hpp"

namespace jkbd {
	// Trigger2MIDI::Trigger2MIDI() {
	// 	forge = new MIDIEventForge();
	// }

	// Trigger2MIDI::~Trigger2MIDI() {}
	
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

			const float thresh = 1.4125375446227555e-05; // -96dB
			if (zero_crossing && (peak[0] > thresh) && (area[0] > thresh)) {
				// Compute the features
				feature[4] = feature[3];
				feature[3] = feature[2];
				feature[2] = feature[1];
				feature[1] = feature[0];
				feature[0] = (peak[0]-peak[1])*4;

				feature[9] = feature[8];
				feature[8] = feature[7];
				feature[7] = feature[6];
				feature[6] = feature[5];
				feature[5] = (area[0]-area[1])/25,

				feature[14] = feature[13];
				feature[13] = feature[12];
				feature[12] = feature[11];
				feature[11] = feature[10];
				feature[10] = (std::fabs(area[0]) + std::fabs(area[1])*2 + std::fabs(area[2])) /
					(std::fabs(width[0]) + std::fabs(width[1])*2 + std::fabs(width[2]));

				// Multi-layer perceptron
				const float bias = 1.0;
				float input_coef[][2] = {
					{ 0.08781958,  0.14427346},
					{ 0.03916803,  0.00817929},
					{ 0.00404339, -0.03656273},
					{-0.11457519,  0.16075008},
					{-0.02217951, -0.13798249},
					{-0.04510742, -0.06000047},
					{-0.10551339,  0.10150451},
					{-0.29324703, -0.26153838},
					{-0.19326483, -0.14095425},
					{-0.04843963,  0.0159393 },
					{-0.07972298, -0.11687325},
					{-0.06327483, -0.11805964},
					{ 0.00130454, -0.07612888},
					{-0.02511146,  0.0083457 },
					{ 0.01779765,  0.05809699}
				};
				float hidden_coef[] = { -1.16724501, 0.6446441 };

				float h0 = 0.72136712*bias;
				for (int i = 0; i < 15; ++i) {
					h0 += input_coef[i][0]*feature[i];
				}
				float h1 = 0.0148684*bias;
				for (int i = 0; i < 15; ++i) {
					h1 += input_coef[i][1]*feature[i];
				}
				float output = 0.83236072*bias + hidden_coef[0]*h0 + hidden_coef[1]*h1;
				
				// Send event
				const uint32_t velocity = (int) (127*output);
				const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range
				
				forge->enqueue_midi_note(42, velocity, frame_time);
			
				// Shift memory and reset the current values
				area[2] = area[1];	
				area[1] = area[0];
				area[0] = 0.0;
				
				peak[1] = peak[0];
				peak[0] = 0.0;
				
				width[2] = width[1];
				width[1] = width[0];
				width[0] = 0;
			} else {
				// Continue accumulating data for the features
				area[0] += x[0];
				peak[0] = (std::fabs(x[0]) > peak[0]) ? std::fabs(x[0]) : peak[0];
				width[0] += 1;
			}

			x[1] = x[0];

			// // TODO: Other channel
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
