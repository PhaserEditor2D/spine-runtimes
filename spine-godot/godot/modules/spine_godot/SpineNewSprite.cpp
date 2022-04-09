/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "SpineNewSprite.h"

#include "SpineEvent.h"
#include "SpineTrackEntry.h"
#include "SpineNewSkeleton.h"

Ref<CanvasItemMaterial> SpineNewSprite::materials[4] = {};
static int sprite_count = 0;

void SpineNewSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_skeleton_data_res", "skeleton_data_res"), &SpineNewSprite::set_skeleton_data_res);
	ClassDB::bind_method(D_METHOD("get_skeleton_data_res"), &SpineNewSprite::get_skeleton_data_res);
	ClassDB::bind_method(D_METHOD("get_skeleton"), &SpineNewSprite::get_skeleton);
	ClassDB::bind_method(D_METHOD("get_animation_state"), &SpineNewSprite::get_animation_state);
	ClassDB::bind_method(D_METHOD("_on_skeleton_data_changed"), &SpineNewSprite::_on_skeleton_data_changed);

	ClassDB::bind_method(D_METHOD("get_bind_slot_nodes"), &SpineNewSprite::get_bind_slot_nodes);
	ClassDB::bind_method(D_METHOD("set_bind_slot_nodes", "v"), &SpineNewSprite::set_bind_slot_nodes);

	ClassDB::bind_method(D_METHOD("get_overlap"), &SpineNewSprite::get_overlap);
	ClassDB::bind_method(D_METHOD("set_overlap", "v"), &SpineNewSprite::set_overlap);

	ClassDB::bind_method(D_METHOD("bone_get_global_transform", "bone_name"), &SpineNewSprite::bone_get_global_transform);
	ClassDB::bind_method(D_METHOD("bone_set_global_transform", "bone_name", "global_transform"), &SpineNewSprite::bone_set_global_transform);

	ClassDB::bind_method(D_METHOD("set_process_mode", "v"), &SpineNewSprite::set_process_mode);
	ClassDB::bind_method(D_METHOD("get_process_mode"), &SpineNewSprite::get_process_mode);

	ClassDB::bind_method(D_METHOD("update_all", "delta"), &SpineNewSprite::_update_all);

	ADD_SIGNAL(MethodInfo("animation_start", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("animation_interrupt", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("animation_end", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("animation_complete", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("animation_dispose", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));
	ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::OBJECT, "animation_state", PROPERTY_HINT_TYPE_STRING, "SpineAnimationState"), PropertyInfo(Variant::OBJECT, "track_entry", PROPERTY_HINT_TYPE_STRING, "SpineTrackEntry"), PropertyInfo(Variant::OBJECT, "event", PROPERTY_HINT_TYPE_STRING, "SpineEvent")));

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skeleton_data_res", PropertyHint::PROPERTY_HINT_RESOURCE_TYPE, "SpineNewSkeletonDataResource"), "set_skeleton_data_res", "get_skeleton_data_res");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "overlap"), "set_overlap", "get_overlap");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "bind_slot_nodes"), "set_bind_slot_nodes", "get_bind_slot_nodes");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "process_mode", PROPERTY_HINT_ENUM, "Process,Physics,Manual"), "set_process_mode", "get_process_mode");

	BIND_ENUM_CONSTANT(ProcessMode::ProcessMode_Process);
	BIND_ENUM_CONSTANT(ProcessMode::ProcessMode_Physics);
	BIND_ENUM_CONSTANT(ProcessMode::ProcessMode_Manual);
}

SpineNewSprite::SpineNewSprite() : overlap(false), process_mode(ProcessMode_Process), skeleton_clipper(nullptr) {
	skeleton_clipper = new spine::SkeletonClipping();

	// One material per blend mode, shared across all sprites.
	if (!materials[0].is_valid()) {
		Ref<CanvasItemMaterial> material_normal(memnew(CanvasItemMaterial));
		material_normal->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX);
		materials[spine::BlendMode_Normal] = material_normal;

		Ref<CanvasItemMaterial> material_additive(memnew(CanvasItemMaterial));
		material_additive->set_blend_mode(CanvasItemMaterial::BLEND_MODE_ADD);
		materials[spine::BlendMode_Additive] = material_additive;

		Ref<CanvasItemMaterial> material_multiply(memnew(CanvasItemMaterial));
		material_multiply->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MUL);
		materials[spine::BlendMode_Multiply] = material_multiply;

		Ref<CanvasItemMaterial> material_screen(memnew(CanvasItemMaterial));
		material_screen->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX);
		materials[spine::BlendMode_Screen] = material_screen;
	}
	sprite_count++;
}

SpineNewSprite::~SpineNewSprite() {
	delete skeleton_clipper;
	sprite_count--;
	if (!sprite_count) {
		for (int i = 0; i < 4; i++) materials[i].unref();
	}
}

void SpineNewSprite::set_skeleton_data_res(const Ref<SpineNewSkeletonDataResource> &s) {
	skeleton_data_res = s;
	_on_skeleton_data_changed();
}
Ref<SpineNewSkeletonDataResource> SpineNewSprite::get_skeleton_data_res() {
	return skeleton_data_res;
}

void SpineNewSprite::_on_skeleton_data_changed() {
	remove_meshes();
	skeleton.unref();
	animation_state.unref();

	if (skeleton_data_res.is_valid()) {
		if (!skeleton_data_res->is_connected("skeleton_data_changed", this, "_on_skeleton_data_changed"))
			skeleton_data_res->connect("skeleton_data_changed", this, "_on_skeleton_data_changed");
	}

	if (skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded()) {
		skeleton = Ref<SpineNewSkeleton>(memnew(SpineNewSkeleton));
		skeleton->set_skeleton_data_res(skeleton_data_res);
		skeleton->set_spine_sprite(this);

		animation_state = Ref<SpineNewAnimationState>(memnew(SpineNewAnimationState));
		animation_state->set_skeleton_data_res(skeleton_data_res);
		if (animation_state->get_spine_object()) animation_state->get_spine_object()->setListener(this);

		animation_state->update(0);
		animation_state->apply(skeleton);
		skeleton->update_world_transform();
		generate_meshes_for_slots(skeleton);

		if (process_mode == ProcessMode_Process) {
			_notification(NOTIFICATION_INTERNAL_PROCESS);
		} else if (process_mode == ProcessMode_Physics) {
			_notification(NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
		}
	}

	property_list_changed_notify();
}

Ref<SpineNewSkeleton> SpineNewSprite::get_skeleton() {
	return skeleton;
}

Ref<SpineNewAnimationState> SpineNewSprite::get_animation_state() {
	return animation_state;
}

void SpineNewSprite::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_process_internal(process_mode == ProcessMode_Process);
			set_physics_process_internal(process_mode == ProcessMode_Physics);
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (process_mode == ProcessMode_Process)
				_update_all(get_process_delta_time());
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (process_mode == ProcessMode_Physics)
				_update_all(get_physics_process_delta_time());
		} break;
	}
}

void SpineNewSprite::_update_all(float delta) {
	if (!(skeleton.is_valid() && animation_state.is_valid()) || mesh_instances.empty())
		return;

	animation_state->update(delta);
	if (!is_visible_in_tree())
		return;

	animation_state->apply(skeleton);
	skeleton->update_world_transform();
	update_meshes(skeleton);
	update();
	update_bind_slot_nodes();
}

void SpineNewSprite::update_bind_slot_nodes() {
	if (animation_state.is_valid() && skeleton.is_valid()) {
		for (size_t i = 0, n = bind_slot_nodes.size(); i < n; ++i) {
			auto a = bind_slot_nodes[i];
			if (a.get_type() == Variant::DICTIONARY) {
				auto d = (Dictionary) a;
				if (d.has("slot_name") && d.has("node_path")) {
					NodePath node_path = d["node_path"];
					Node *node = get_node_or_null(node_path);
					if (node && node->is_class("Node2D")) {
						Node2D *node2d = (Node2D *) node;

						String slot_name = d["slot_name"];
						auto slot = skeleton->find_slot(slot_name);
						if (slot.is_valid()) {
							auto bone = slot->get_bone();
							if (bone.is_valid()) {
								update_bind_slot_node_transform(bone, node2d);
								update_bind_slot_node_draw_order(slot_name, node2d);
							}
						}
					}
				}
			} else if (a.get_type() == Variant::ARRAY) {
				auto as = (Array) a;// 0: slot_name, 1: node_path
				if (as.size() >= 2 && as[0].get_type() == Variant::STRING && as[1].get_type() == Variant::NODE_PATH) {
					NodePath node_path = as[1];
					Node *node = get_node_or_null(node_path);
					if (node && node->is_class("Node2D")) {
						Node2D *node2d = (Node2D *) node;

						String slot_name = as[0];
						auto slot = skeleton->find_slot(slot_name);
						if (slot.is_valid()) {
							auto bone = slot->get_bone();
							if (bone.is_valid()) {
								update_bind_slot_node_transform(bone, node2d);
								update_bind_slot_node_draw_order(slot_name, node2d);
							}
						}
					}
				}
			}
		}
	}
}

void SpineNewSprite::update_bind_slot_node_transform(Ref<SpineBone> bone, Node2D *node2d) {
	bone->apply_world_transform_2d(node2d);
}

void SpineNewSprite::update_bind_slot_node_draw_order(const String &slot_name, Node2D *node2d) {
	auto mesh_ins = find_node(slot_name);
	if (mesh_ins) {
		auto pos = mesh_ins->get_index();

		// get child
		auto node = find_child_node_by_node(node2d);
		if (node && node->get_index() != pos + 1) {
			move_child(node, pos + 1);
		}
	}
}
Node *SpineNewSprite::find_child_node_by_node(Node *node) {
	if (node == nullptr) return nullptr;
	while (node && node->get_parent() != this) node = node->get_parent();
	return node;
}

void SpineNewSprite::generate_meshes_for_slots(Ref<SpineNewSkeleton> skeleton_ref) {
	auto skeleton = skeleton_ref->get_spine_object();
	for (int i = 0, n = skeleton->getSlots().size(); i < n; i++) {

		auto mesh_instance = memnew(MeshInstance2D);
		mesh_instance->set_position(Vector2(0, 0));
		mesh_instance->set_material(materials[spine::BlendMode_Normal]);

		add_child(mesh_instance);
		mesh_instance->set_owner(this);
		mesh_instances.push_back(mesh_instance);
	}
}

void SpineNewSprite::remove_meshes() {
	for (size_t i = 0; i < mesh_instances.size(); ++i) {
		remove_child(mesh_instances[i]);
		memdelete(mesh_instances[i]);
	}
	mesh_instances.clear();
}

#define TEMP_COPY(t, get_res)                   \
	do {                                        \
		auto &temp_uvs = get_res;               \
		t.setSize(temp_uvs.size(), 0);          \
		for (size_t j = 0; j < t.size(); ++j) { \
			t[j] = temp_uvs[j];                 \
		}                                       \
	} while (false);

void SpineNewSprite::update_meshes(Ref<SpineNewSkeleton> s) {
	static const unsigned short VERTEX_STRIDE = 2;
	static unsigned short quad_indices[] = {0, 1, 2, 2, 3, 0};

	auto sk = s->get_spine_object();
	for (size_t i = 0, n = sk->getSlots().size(); i < n; ++i) {
		spine::Vector<float> vertices;
		spine::Vector<float> uvs;
		spine::Vector<unsigned short> indices;

		spine::Slot *slot = sk->getDrawOrder()[i];

		spine::Attachment *attachment = slot->getAttachment();
		if (!attachment) {
			mesh_instances[i]->set_visible(false);

			skeleton_clipper->clipEnd(*slot);
			continue;
		}
		mesh_instances[i]->set_visible(true);

		spine::Color skeleton_color = sk->getColor();
		spine::Color slot_color = slot->getColor();
		spine::Color tint(skeleton_color.r * slot_color.r, skeleton_color.g * slot_color.g, skeleton_color.b * slot_color.b, skeleton_color.a * slot_color.a);

		Ref<Texture> tex;
		Ref<Texture> normal_tex;
		size_t v_num = 0;

		if (attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
			spine::RegionAttachment *region_attachment = (spine::RegionAttachment *) attachment;

			auto p_spine_renderer_object = (SpineRendererObject *) ((spine::AtlasRegion *) region_attachment->getRendererObject())->page->getRendererObject();
			tex = p_spine_renderer_object->texture;
			normal_tex = p_spine_renderer_object->normal_map;

			v_num = 4;
			vertices.setSize(v_num * VERTEX_STRIDE, 0);

			region_attachment->computeWorldVertices(*slot, vertices, 0);

			TEMP_COPY(uvs, region_attachment->getUVs());

			indices.setSize(sizeof(quad_indices) / sizeof(unsigned short), 0);
			for (size_t j = 0, qn = indices.size(); j < qn; ++j) {
				indices[j] = quad_indices[j];
			}

			auto attachment_color = region_attachment->getColor();
			tint.r *= attachment_color.r;
			tint.g *= attachment_color.g;
			tint.b *= attachment_color.b;
			tint.a *= attachment_color.a;
		} else if (attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
			spine::MeshAttachment *mesh = (spine::MeshAttachment *) attachment;

			auto p_spine_renderer_object = (SpineRendererObject *) ((spine::AtlasRegion *) mesh->getRendererObject())->page->getRendererObject();
			tex = p_spine_renderer_object->texture;
			normal_tex = p_spine_renderer_object->normal_map;

			v_num = mesh->getWorldVerticesLength() / VERTEX_STRIDE;
			vertices.setSize(mesh->getWorldVerticesLength(), 0);

			mesh->computeWorldVertices(*slot, vertices);
			TEMP_COPY(uvs, mesh->getUVs());
			TEMP_COPY(indices, mesh->getTriangles());

			auto attachment_color = mesh->getColor();
			tint.r *= attachment_color.r;
			tint.g *= attachment_color.g;
			tint.b *= attachment_color.b;
			tint.a *= attachment_color.a;
		} else if (attachment->getRTTI().isExactly(spine::ClippingAttachment::rtti)) {
			auto clip = (spine::ClippingAttachment *) attachment;
			skeleton_clipper->clipStart(*slot, clip);
			continue;
		} else {
			skeleton_clipper->clipEnd(*slot);
			continue;
		}

		auto mesh_ins = mesh_instances[i];
		VisualServer::get_singleton()->canvas_item_clear(mesh_ins->get_canvas_item());

		if (skeleton_clipper->isClipping()) {
			skeleton_clipper->clipTriangles(vertices, indices, uvs, VERTEX_STRIDE);

			if (skeleton_clipper->getClippedTriangles().size() == 0) {
				skeleton_clipper->clipEnd(*slot);
				continue;
			}

			auto &clipped_vertices = skeleton_clipper->getClippedVertices();
			v_num = clipped_vertices.size() / VERTEX_STRIDE;
			auto &clipped_uvs = skeleton_clipper->getClippedUVs();
			auto &clipped_indices = skeleton_clipper->getClippedTriangles();

			if (indices.size() > 0) {
				Vector<Vector2> p_points, p_uvs;
				Vector<Color> p_colors;
				Vector<int> p_indices;
				p_points.resize(v_num);
				p_uvs.resize(v_num);
				p_colors.resize(v_num);
				for (size_t j = 0; j < v_num; j++) {
					p_points.set(j, Vector2(clipped_vertices[j * VERTEX_STRIDE], -clipped_vertices[j * VERTEX_STRIDE + 1]));
					p_uvs.set(j, Vector2(clipped_uvs[j * VERTEX_STRIDE], clipped_uvs[j * VERTEX_STRIDE + 1]));
					p_colors.set(j, Color(tint.r, tint.g, tint.b, tint.a));
				}
				p_indices.resize(clipped_indices.size());
				for (size_t j = 0; j < clipped_indices.size(); ++j) {
					p_indices.set(j, clipped_indices[j]);
				}

				VisualServer::get_singleton()->canvas_item_add_triangle_array(mesh_ins->get_canvas_item(),
																			  p_indices,
																			  p_points,
																			  p_colors,
																			  p_uvs,
																			  Vector<int>(),
																			  Vector<float>(),
																			  tex.is_null() ? RID() : tex->get_rid(),
																			  -1,
																			  normal_tex.is_null() ? RID() : normal_tex->get_rid());
			}
		} else {
			if (indices.size() > 0) {
				Vector<Vector2> p_points, p_uvs;
				Vector<Color> p_colors;
				Vector<int> p_indices;
				p_points.resize(v_num);
				p_uvs.resize(v_num);
				p_colors.resize(v_num);
				for (size_t j = 0; j < v_num; j++) {
					p_points.set(j, Vector2(vertices[j * VERTEX_STRIDE], -vertices[j * VERTEX_STRIDE + 1]));
					p_uvs.set(j, Vector2(uvs[j * VERTEX_STRIDE], uvs[j * VERTEX_STRIDE + 1]));
					p_colors.set(j, Color(tint.r, tint.g, tint.b, tint.a));
				}
				p_indices.resize(indices.size());
				for (size_t j = 0; j < indices.size(); ++j) {
					p_indices.set(j, indices[j]);
				}

				VisualServer::get_singleton()->canvas_item_add_triangle_array(mesh_ins->get_canvas_item(),
																			  p_indices,
																			  p_points,
																			  p_colors,
																			  p_uvs,
																			  Vector<int>(),
																			  Vector<float>(),
																			  tex.is_null() ? RID() : tex->get_rid(),
																			  -1,
																			  normal_tex.is_null() ? RID() : normal_tex->get_rid());
			}
		}
		skeleton_clipper->clipEnd(*slot);

		if (mesh_ins->get_material()->is_class("CanvasItemMaterial")) {
			mesh_ins->set_material(materials[slot->getData().getBlendMode()]);
		}
	}
	skeleton_clipper->clipEnd();
}

void SpineNewSprite::callback(spine::AnimationState *state, spine::EventType type, spine::TrackEntry *entry, spine::Event *event) {
	Ref<SpineTrackEntry> gd_entry(nullptr);
	Ref<SpineEvent> gd_event(nullptr);

	if (entry) {
		gd_entry = Ref<SpineTrackEntry>(memnew(SpineTrackEntry));
		gd_entry->set_spine_object(entry);
	}
	if (event) {
		gd_event = Ref<SpineEvent>(memnew(SpineEvent));
		gd_event->set_spine_object(event);
	}

	switch (type) {
		case spine::EventType_Start: {
			emit_signal("animation_start", animation_state, gd_entry, gd_event);
		} break;
		case spine::EventType_Interrupt: {
			emit_signal("animation_interrupt", animation_state, gd_entry, gd_event);
		} break;
		case spine::EventType_End: {
			emit_signal("animation_end", animation_state, gd_entry, gd_event);
		} break;
		case spine::EventType_Complete: {
			emit_signal("animation_complete", animation_state, gd_entry, gd_event);
		} break;
		case spine::EventType_Dispose: {
			emit_signal("animation_dispose", animation_state, gd_entry, gd_event);
		} break;
		case spine::EventType_Event: {
			emit_signal("animation_event", animation_state, gd_entry, gd_event);
		} break;
	}
}

Array SpineNewSprite::get_bind_slot_nodes() {
	return bind_slot_nodes;
}

void SpineNewSprite::set_bind_slot_nodes(Array v) {
	bind_slot_nodes = v;
}

bool SpineNewSprite::get_overlap() {
	return overlap;
}

void SpineNewSprite::set_overlap(bool v) {
	overlap = v;
}

Transform2D SpineNewSprite::bone_get_global_transform(const String &bone_name) {
	if (!animation_state.is_valid() && !skeleton.is_valid()) {
		return get_global_transform();
	}
	auto bone = skeleton->find_bone(bone_name);
	if (!bone.is_valid()) {
		print_error(vformat("Bone: '%s' not found.", bone_name));
		return get_global_transform();
	}
	return bone->get_godot_global_transform();
}

void SpineNewSprite::bone_set_global_transform(const String &bone_name, Transform2D transform) {
	if (!animation_state.is_valid() && !skeleton.is_valid()) {
		return;
	}
	auto bone = skeleton->find_bone(bone_name);
	if (!bone.is_valid()) {
		return;
	}
	bone->set_godot_global_transform(transform);
}

SpineNewSprite::ProcessMode SpineNewSprite::get_process_mode() {
	return process_mode;
}

void SpineNewSprite::set_process_mode(SpineNewSprite::ProcessMode v) {
	process_mode = v;
	set_process_internal(process_mode == ProcessMode_Process);
	set_physics_process_internal(process_mode == ProcessMode_Physics);
}

void SpineNewSprite::_get_property_list(List<PropertyInfo> *p_list) const {
	Vector<String> animations;
	Vector<String> skins;
	if (skeleton_data_res.is_valid()) {
		skeleton_data_res->get_animation_names(animations);
		skeleton_data_res->get_skin_names(skins);
	}
	animations.insert(0, "- None -");

	PropertyInfo animationListProperty;
	animationListProperty.name = "Preview animation";
	animationListProperty.type = Variant::STRING;
	animationListProperty.hint_string = String(",").join(animations);
	animationListProperty.hint = PROPERTY_HINT_ENUM;
	animationListProperty.usage = PROPERTY_USAGE_EDITOR;
	p_list->push_back(animationListProperty);

	PropertyInfo skinListProperty;
	skinListProperty.name = "Preview skin";
	skinListProperty.type = Variant::STRING;
	skinListProperty.hint_string = String(",").join(skins);
	skinListProperty.hint = PROPERTY_HINT_ENUM;
	skinListProperty.usage = PROPERTY_USAGE_EDITOR;
	p_list->push_back(skinListProperty);
}

bool SpineNewSprite::_get(const StringName &p_property, Variant &r_value) const {
	return false;
}

bool SpineNewSprite::_set(const StringName &p_property, const Variant &p_value) {
	if (p_property == "Preview animation") {
		if (animation_state.is_valid() && skeleton.is_valid()) {
			auto animName = p_value.operator String();
			skeleton->set_to_setup_pose();
			if (skeleton->get_skeleton_data_res()->find_animation(animName).is_valid()) {
				animation_state->set_animation(animName, true, 0);
			} else {
				animation_state->clear_tracks();
			}
		}
	}

	if (p_property == "Preview skin") {
		if (animation_state.is_valid() && skeleton.is_valid()) {
			auto skinName = p_value.operator String();
			if (skeleton->get_skeleton_data_res()->find_skin(skinName).is_valid()) {
				skeleton->set_skin_by_name(skinName);
			} else {
				skeleton->set_skin(nullptr);
			}
			skeleton->set_to_setup_pose();
		}
	}
	return false;
}

#ifdef TOOLS_ENABLED
Rect2 SpineNewSprite::_edit_get_rect() const {
	if (skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded()) {
		auto data = skeleton_data_res->get_skeleton_data();
		return Rect2(data->getX(), data->getY() - data->getHeight(), data->getWidth(), data->getHeight());
	}

	return Node2D::_edit_get_rect();
}

bool SpineNewSprite::_edit_use_rect() const {
	return skeleton_data_res.is_valid() && skeleton_data_res->is_skeleton_data_loaded();
}
#endif