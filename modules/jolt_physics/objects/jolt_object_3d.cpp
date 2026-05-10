/**************************************************************************/
/*  jolt_object_3d.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "jolt_object_3d.h"

#include "../jolt_physics_server_3d.h"
#include "../jolt_project_settings.h"
#include "../spaces/jolt_layers.h"
#include "../spaces/jolt_space_3d.h"
#include "jolt_group_filter.h"

void JoltObject3D::_remove_from_space() {
	if (!in_space()) {
		return;
	}

	// PlayFight diagnostic: trace removals for bodies in the rollback range so
	// we can see what is yanking them back out of the space after `_add_to_space`
	// succeeded. Filter by ID range to avoid log spam from level statics.
	const uint32_t live_id = jolt_body->GetID().GetIndexAndSequenceNumber();
	if (live_id >= 1024 && live_id < 0x800000) {
		print_line(vformat("[remove_from_space] live=%d keep_alive=%d", (int)live_id, (int)has_pending_jolt_id()));
	}

	if (has_pending_jolt_id()) {
		// PlayFight: rollback body. Keep its slot reserved in BodyManager so
		// Jolt's auto-allocator can't grab it before the next disable/enable
		// cycle re-adds it. Final destruction is via free_body/free_soft_body.
		space->remove_object_keep_alive(jolt_body->GetID());
		kept_alive_in_space = space;
	} else {
		space->remove_object(jolt_body->GetID());
	}
	jolt_body = nullptr;
}

void JoltObject3D::_reset_space() {
	ERR_FAIL_NULL(space);

	_space_changing();
	_remove_from_space();
	_add_to_space();
	_space_changed();
}

void JoltObject3D::_update_object_layer() {
	if (!in_space()) {
		return;
	}

	space->get_body_iface().SetObjectLayer(jolt_body->GetID(), _get_object_layer());
}

void JoltObject3D::_collision_layer_changed() {
	_update_object_layer();
}

void JoltObject3D::_collision_mask_changed() {
	_update_object_layer();
}

JoltObject3D::JoltObject3D(ObjectType p_object_type) :
		object_type(p_object_type) {
}

JoltObject3D::~JoltObject3D() = default;

Object *JoltObject3D::get_instance() const {
	return ObjectDB::get_instance(instance_id);
}

void JoltObject3D::set_space(JoltSpace3D *p_space) {
	// PlayFight diagnostic: trace set_space transitions, but only for bodies
	// that have a pending rollback ID hint set (otherwise level statics flood
	// the log). Bodies in our rollback set always have a pending hint at the
	// moment set_space(real_space) is called the first time.
	if (has_pending_jolt_id()) {
		print_line(vformat("[set_space] pending=%d cur_space_set=%d new_space_set=%d",
				(int)pending_jolt_id.GetIndexAndSequenceNumber(),
				space != nullptr ? 1 : 0,
				p_space != nullptr ? 1 : 0));
	}

	if (space == p_space) {
		return;
	}

	_space_changing();

	if (space != nullptr) {
		_remove_from_space();
	}

	space = p_space;

	if (space != nullptr) {
		_add_to_space();
		// PlayFight: body is back in an active space; clear the kept-alive
		// pointer (only meaningful when the body is in BodyManager but not
		// in any active space).
		kept_alive_in_space = nullptr;
	}

	_space_changed();
}

void JoltObject3D::set_collision_layer(uint32_t p_layer) {
	if (p_layer == collision_layer) {
		return;
	}

	collision_layer = p_layer;

	_collision_layer_changed();
}

void JoltObject3D::set_collision_mask(uint32_t p_mask) {
	if (p_mask == collision_mask) {
		return;
	}

	collision_mask = p_mask;

	_collision_mask_changed();
}

bool JoltObject3D::can_collide_with(const JoltObject3D &p_other) const {
	return (collision_mask & p_other.get_collision_layer()) != 0;
}

bool JoltObject3D::can_interact_with(const JoltObject3D &p_other) const {
	if (const JoltBody3D *other_body = p_other.as_body()) {
		return can_interact_with(*other_body);
	} else if (const JoltArea3D *other_area = p_other.as_area()) {
		return can_interact_with(*other_area);
	} else if (const JoltSoftBody3D *other_soft_body = p_other.as_soft_body()) {
		return can_interact_with(*other_soft_body);
	} else {
		ERR_FAIL_V_MSG(false, vformat("Unhandled object type: '%d'. This should not happen. Please report this.", p_other.get_type()));
	}
}

String JoltObject3D::to_string() const {
	static const String fallback_name = "<unknown>";

	if (JoltPhysicsServer3D::get_singleton()->is_on_separate_thread()) {
		return fallback_name; // Calling `Object::to_string` is not thread-safe.
	}

	Object *instance = get_instance();
	return instance != nullptr ? instance->to_string() : fallback_name;
}
