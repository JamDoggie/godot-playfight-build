/**************************************************************************/
/*  jolt_state_recorder.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "jolt_state_recorder.h"

#include "core/string/print_string.h"

JoltStateRecorder::JoltStateRecorder(const PackedByteArray &p_data) {
	load_from_packed_byte_array(p_data);
}

void JoltStateRecorder::WriteBytes(const void *inData, size_t inNumBytes) {
	if (inNumBytes == 0) {
		return;
	}
	const uint8_t *src = static_cast<const uint8_t *>(inData);
	uint64_t old_size = data.size();
	data.resize(old_size + inNumBytes);
	memcpy(data.ptr() + old_size, src, inNumBytes);
}

void JoltStateRecorder::ReadBytes(void *outData, size_t inNumBytes) {
	if (inNumBytes == 0) {
		return;
	}
	if (read_cursor + inNumBytes > data.size()) {
		failed = true;
		// Zero out the destination so the caller doesn't read uninitialized memory
		// even when we've gone past the end of the buffer.
		memset(outData, 0, inNumBytes);
		return;
	}
	memcpy(outData, data.ptr() + read_cursor, inNumBytes);
	read_cursor += inNumBytes;
}

bool JoltStateRecorder::IsEOF() const {
	return read_cursor >= data.size();
}

bool JoltStateRecorder::IsFailed() const {
	return failed;
}

PackedByteArray JoltStateRecorder::to_packed_byte_array() const {
	PackedByteArray out;
	out.resize(data.size());
	if (data.size() > 0) {
		memcpy(out.ptrw(), data.ptr(), data.size());
	}
	return out;
}

void JoltStateRecorder::load_from_packed_byte_array(const PackedByteArray &p_data) {
	data.resize(p_data.size());
	if (p_data.size() > 0) {
		memcpy(data.ptr(), p_data.ptr(), p_data.size());
	}
	read_cursor = 0;
	failed = false;
}

void JoltStateRecorder::rewind() {
	read_cursor = 0;
	failed = false;
}

void JoltStateRecorder::clear() {
	data.clear();
	read_cursor = 0;
	failed = false;
}
