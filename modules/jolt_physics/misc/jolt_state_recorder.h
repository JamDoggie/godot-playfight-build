/**************************************************************************/
/*  jolt_state_recorder.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/templates/local_vector.h"
#include "core/variant/variant.h"

#include "Jolt/Jolt.h"

#include "Jolt/Physics/StateRecorder.h"

// PlayFight: Vector<uint8_t>-backed JPH::StateRecorder so we can save/restore
// the full Jolt physics-system state (bodies, contacts, constraints, global)
// for rollback netcode. Behaves the same as Jolt's bundled StateRecorderImpl
// (which uses a std::stringstream) but exposes a flat byte buffer suitable
// for shipping over the wire / packing into a Godot PackedByteArray.
class JoltStateRecorder : public JPH::StateRecorder {
public:
	JoltStateRecorder() = default;
	explicit JoltStateRecorder(const PackedByteArray &p_data);

	// JPH::StreamOut overrides
	virtual void WriteBytes(const void *inData, size_t inNumBytes) override;

	// JPH::StreamIn overrides
	virtual void ReadBytes(void *outData, size_t inNumBytes) override;
	virtual bool IsEOF() const override;
	virtual bool IsFailed() const override;

	// Access to the underlying buffer for serialization.
	PackedByteArray to_packed_byte_array() const;
	void load_from_packed_byte_array(const PackedByteArray &p_data);

	// Reset the read cursor so the same recorder can be read multiple times.
	void rewind();

	// Drop both buffer and cursor.
	void clear();

	uint64_t size() const { return data.size(); }

private:
	LocalVector<uint8_t> data;
	uint64_t read_cursor = 0;
	mutable bool failed = false;
};
