#include "Trigger2MIDI.hpp"

namespace jkbd {
	float logistic(float x) {
		return 1 / (1 + exp(-x));
	}

	float relu(float x) {
		return std::fmax(0, x);
	}

	bool is_zero_crossing(float x0, float x1) {
		return (x0 >= 0.0f) && (x1 < 0.0f) || (x0 < 0.0f) && (x1 >= 0.0f);
	}
	
	void Trigger2MIDI::sample_rate(double sr) {
		assert((8000.0 <= sr) and (sr <= 192000.0));
		Trigger2MIDI::sr = sr;
	}
	
	void Trigger2MIDI::run(uint32_t n_samples) {
		forge->prepare(midi_out);

		const float sampling_freq = 44100.0;
		const float fConst0 = std::tan((873.362732f / std::min<float>(192000.0f, std::max<float>(1.0f, float(sampling_freq)))));
		const float fConst1 = (1.0f / fConst0);
		const float fConst2 = (1.0f / (((fConst1 + 1.41421354f) / fConst0) + 1.0f));
		const float fConst3 = (((fConst1 + -1.41421354f) / fConst0) + 1.0f);
		const float fConst4 = (2.0f * (1.0f - (1.0f / (fConst0*fConst0))));
		
		for (uint32_t n = 0; n < n_samples; ++n) {
			// 2nd order Butterworth lowpass filter at 278Hz.
			fRec0[0] = (trigger_in[Trigger2MIDI::Port::SNARE][n] - (fConst2 * ((fConst3 * fRec0[2]) + (fConst4 * fRec0[1]))));
			x[0] = fConst2 * (fRec0[2] + (fRec0[0] + (2.0f * fRec0[1])));
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];

			if (is_zero_crossing(x[0], x[1])) {
				const float thresh = 0.00025118864315095795; // -72dB
				if (peak[0] > thresh) {
					for (int i = 0; i < 12; ++i) {
						feature[i] = peak[i];
					}
									       
					// Multi-layer perceptron. 12
					// inputs, 2 hidden layers and
					// 1 output. Decide if it is
					// an onset or not.
					const float bias = 1.0;
					float input_coef[][2] = {
						{ 0.55564018,  0.74399126},
						{-0.36071648,  0.06788682},
						{-0.96500742,  2.40568343},
						{ 1.10200468, -0.85314039},
						{-0.91889875,  1.39357203},
						{-0.12007612,  0.24577365},
						{ 1.87284722, -1.34054456},
						{-1.07809225,  2.35744059},
						{-0.39912328,  0.86952983},
						{ 0.40117409, -0.13387597},
						{ 2.22646752, -2.01398985},
						{-0.5373054,   1.08200508}
					};
					
					float hidden_coef[][2] = {
						{ 1.4522158 }, {-3.23596404}
					};
					
					float h0 = 0.30643672*bias;
					for (int i = 0; i < 12; ++i) {
						h0 += input_coef[i][0]*feature[i];
					}
					h0 = relu(h0);
					float h1 = 1.14179894*bias;
					for (int i = 0; i < 12; ++i) {
						h1 += input_coef[i][1]*feature[i];
					}
					h1 = relu(h1);
					
					float o0 = -1.52301205*bias;
					o0 += hidden_coef[0][0]*h0;
					o0 += hidden_coef[0][1]*h1;
					o0 = logistic(o0);
					
					// Send event
					float onset_peak = 0.0;
					for (int i = 0; i < 12; ++i) {
						onset_peak += peak[i];
					}		
					std::cout << o0 << ", " << onset_peak << std::endl;
					
					const uint32_t velocity = (int) (127*(onset_peak));
					const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range
					//forge->enqueue_midi_note(41, velocity, frame_time);
					
					if (o0 > 0.5) {
						forge->enqueue_midi_note(42, velocity, frame_time);
					}
					
					// Shift memory and reset the current values
					for (int i = 12; i > 0; --i) {
						peak[i] = peak[i-1];
					}
					peak[0] = std::fabs(x[0]);
				} else {
					// SKIP
				}
				
			} else {
				// Continue accumulating data for the features				
				peak[0] = (std::fabs(x[0]) > peak[0]) ? std::fabs(x[0]) : peak[0];
			}

			x[1] = x[0];

			fVec0[2] = fVec0[1];
			fVec0[1] = fVec0[0];
			for (int j0 = 4; (j0 > 0); j0 = (j0 - 1)) {
				fVec1[j0] = fVec1[(j0 - 1)];
			}
			for (int j1 = 4; (j1 > 0); j1 = (j1 - 1)) {
				fVec2[j1] = fVec2[(j1 - 1)];
			}
		       
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
