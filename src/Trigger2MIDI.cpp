#include "Trigger2MIDI.hpp"

namespace jkbd {
	inline float decibel_to_coef(float gain) {
		return ((gain) > -90.0f ? powf(10.0f, (gain) * 0.05f) : 0.0f);
	}
	
	void Trigger2MIDI::sample_rate(double sr) {
		// TODO: assert 0 < sr <= 192000.0 ?
		Trigger2MIDI::sr = sr;
	}
	
	void Trigger2MIDI::run(uint32_t n_samples) {
		for (uint32_t pos = 0; pos < n_samples; ++pos) {
			midi_out[pos] = 0.0;
			snare[pos] = 0.0;			
		}
	}
	
	static LV2_Handle instantiate(const LV2_Descriptor*     descriptor,
				      double                    rate,
				      const char*               bundle_path,
				      const LV2_Feature* const* features)
	{
		Trigger2MIDI* self = new Trigger2MIDI();
		self->sample_rate(rate);
		return static_cast<LV2_Handle>(self);
	}
	
	static void connect_port(LV2_Handle instance, uint32_t port, void* data)
	{
		Trigger2MIDI* self = static_cast<Trigger2MIDI*>(instance);
		switch (static_cast<Trigger2MIDI::Port>(port)) {
		case Trigger2MIDI::Port::SNARE:
			self->snare = static_cast<float*>(data);
			break;
		case Trigger2MIDI::Port::MIDI_OUT:
			self->midi_out = static_cast<float*>(data);
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
