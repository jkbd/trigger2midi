#include "Trigger2MIDI.hpp"

namespace jkbd {
	void Trigger2MIDI::sample_rate(double sr) {
		assert((8000.0 <= sr) and (sr <= 192000.0));
		Trigger2MIDI::sr = sr;
	}

	
	/**
	 * The identity function. `input` is one block of
	 * non-interleaved single channel audio stream of length
	 * `n_samples`.
	 */
	void id(float input[], float output[], uint32_t n_samples) {
		assert(input != NULL);
		assert(output != NULL);
		for (uint32_t n = 0; n < n_samples; ++n) {
			output[n] = input[n];
		}
	}

	
	/**
	 * Attack and relase in seconds.
	 */
	void transient_envelope(float input[], float output[], uint32_t n_samples, float sample_rate,
				transient_envelope_state_t *state,
				float attack, float release) {
		assert(input != NULL);
		assert(output != NULL);
		assert(state != NULL);
		assert(attack > 0);
		assert(release > 0);
		assert(attack < release);

		float *const r0 = state->r0;
		float *const r1 = state->r1;
		
		const float T = 1.0f/sample_rate;

		const float s0 = std::exp(0.0f - (T/release));
		const float s1 = (1.0f - s0);
		const float s2 = std::exp(0.0f - (T/attack));
		const float s3 = (1.0f - s2);
		
		for (uint32_t n = 0; n < n_samples; ++n) {
			const float x0 = std::fabs(input[n]);
			r0[0] = std::max<float>(x0, ((s0 * r0[1]) + (s1 * x0)));
			r1[0] = ((s2 * r1[1]) + (s3 * r0[0]));
			output[n] = std::max<float>(0.0f, (r0[0] - r1[0]));
			r0[1] = r0[0];
			r1[1] = r1[0];
		}
	}


	/**
	 * Emit a MIDI Note On event shortly after the onset. `input`
	 * must be a transient envelope.
	 */
	void onset_noteon(float input[], float envelope[], MIDIEventForge* forge, uint32_t n_samples, float sample_rate,
			  onset_noteon_state_t *state,
			  float threshold, int note) {
		assert(input != NULL);
		assert(forge != NULL);
		assert(state != NULL);
		assert((0.0 < threshold) && (threshold < 1.0));
		assert((0 <= note) && (note <= 127));

		float *const x = state->x;
		float *const e = state->e;
		float *const peak = state->peak;
		
		for (uint32_t n = 0; n < n_samples; ++n) {
			x[0] = input[n];
			e[0] = envelope[n];
			
			// Analyse onset window
			if (e[0] > threshold) {
				// While the amplitude envelope is
				// high, measure the signal peak.
				peak[0] = (std::fabs(x[0]) > peak[0]) ? std::fabs(x[0]) : peak[0];
			} else {
				// If the amplitude envelope is low
				// but was high before, emit a MIDI
				// Note-On event.
				if (e[1] > threshold) {
					// Send event
					const uint32_t velocity = std::min<int>(127, std::max<int>(0, (int)(127 * peak[0])));
					const int64_t frame_time = n; //-(zero[0]+zero[1]); // TODO guard range

					// DEBUG
					float dB = 20*log10(peak[0]);
					std::cerr << "HIT! " << 127*1.2*peak[0] << ", " << peak[0] << ", " << dB << std::endl;
					
					forge->enqueue_midi_note(note, velocity, frame_time);

					// Reset the measurement.
					peak[1] = peak[0];
					peak[0] = std::fabs(x[0]);
				} else {
					// Nothing to do
				}
			}
			
			x[1] = x[0];
			e[1] = e[0];
		}
	}

	
	void Trigger2MIDI::run(uint32_t n_samples) {
		forge->prepare(midi_out);
	       	
		float* input = trigger_in[Trigger2MIDI::Port::SNARE];
		float* output = cv_out;

		// Copy input to output.
		id(input, output, n_samples);

		// Process inplace in the output buffer.
		const float attack = 128.0/Trigger2MIDI::sr;
		const float release = 3*attack;
		transient_envelope(output, output, n_samples, Trigger2MIDI::sr,
				   Trigger2MIDI::transient_envelope_state, attack, release);

		// Send MIDI
		//const float threshold = std::pow(10.0f, (0.05f * (*dynamic_range)));
		const float threshold = 0.2;
		const int note = static_cast<int>(*note_number);
		onset_noteon(input, output, forge, n_samples, Trigger2MIDI::sr,
			     Trigger2MIDI::onset_noteon_state, threshold, note);
		
		forge->finish();
	}
	
	
	static LV2_Handle instantiate(const LV2_Descriptor*     descriptor,
				      double                    rate,
				      const char*               bundle_path,
				      const LV2_Feature* const* features)
	{
		Trigger2MIDI* self = new Trigger2MIDI();
		self->sample_rate(rate);
		self->transient_envelope_state = new transient_envelope_state_t;
		self->onset_noteon_state = new onset_noteon_state_t;

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
		Trigger2MIDI* self = static_cast<Trigger2MIDI*>(instance);
		delete self->transient_envelope_state;
		delete self->onset_noteon_state;
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
