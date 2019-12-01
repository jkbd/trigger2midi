#include "MIDIEventForge.hpp"

namespace jkbd {
	MIDIEventForge::MIDIEventForge(LV2_URID_Map* map) {
	 	assert(map != nullptr);
		
		midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
		atom_Sequence  = map->map(map->handle, LV2_ATOM__Sequence);
		lv2_atom_forge_init(&forge, map);
	}
	
	//~MIDIEventForge() {}

	void MIDIEventForge::prepare(LV2_Atom_Sequence* sink) {
	 	assert(sink != nullptr);
		
		const uint32_t capacity = sink->atom.size;
		lv2_atom_forge_set_buffer(&forge, (uint8_t*)(sink), capacity);
		lv2_atom_forge_sequence_head(&forge, &frame, 0);
	}

	void MIDIEventForge::enqueue_midi_note(int8_t velocity, int64_t frame_time) {
		uint8_t raw_midi[3];
		const uint8_t channel = 0x09;
		raw_midi[0] = (channel & 0x0f) | ((velocity & 0x7f) ? 0x90 : 0x80);
		//raw_midi[1] = velocity != 0 ? (static_cast<LV2_Handle>(*m_note) & 0x7f) : midi_note_value;
		raw_midi[1] = 42; // note number
		raw_midi[2] = velocity & 0x7f;
		// if (velocity != 0) {
		// 	midi_note_value = raw_midi[1];
		// } else {
		// 	// Nothing
		// }
		enqueue(raw_midi, frame_time);
	}
		
	void MIDIEventForge::finish() {
		lv2_atom_forge_pop(&forge, &frame);
	}

	void MIDIEventForge::enqueue(uint8_t raw_midi[3], int64_t frame_time) {
		LV2_Atom midiatom;
		midiatom.type = midi_MidiEvent;
		midiatom.size = 3;
		lv2_atom_forge_frame_time(&forge, frame_time);
		lv2_atom_forge_raw(&forge, &midiatom, sizeof(LV2_Atom));
		lv2_atom_forge_raw(&forge, raw_midi, 3);
		lv2_atom_forge_pad(&forge, sizeof(LV2_Atom) + midiatom.size);
	}

}
