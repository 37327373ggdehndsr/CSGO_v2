#include "../features.h"

animation::animation(c_cs_player* player){
	const auto weapon = player->get_active_weapon();

	this->player = player;
	index = player->get_index();
	dormant = player->is_dormant();
	velocity = player->get_velocity();
	origin = player->get_origin();
	abs_origin = player->get_abs_origin();
	obb_mins = player->get_collideable()->obb_mins();
	obb_maxs = player->get_collideable()->obb_maxs();
	memcpy(layers, player->get_animlayers(), sizeof(anim_layer_t) * 13);
	player->get_pose_parameters(poses);
	anim_state = player->get_anim_state();
	sim_time = player->get_sim_time();
	interp_time = 0.f;
	last_shot_time = weapon ? weapon->get_last_shot_time() : 0.f;
	duck = player->get_duck_amount();
	lby = player->get_lby();
	eye_angles = player->get_eye_angles();
	abs_ang = player->get_abs_angles();
	flags = player->get_flags();
	eflags = player->get_eflags();
	effects = player->get_effects();

	lag = TIME_TO_TICKS(player->get_sim_time() - player->get_old_sim_time());

	// animations are off when we enter pvs, we do not want to shoot yet.
	valid = lag >= 0 && lag <= 17;

	// clamp it so we don't interpolate too far : )
	lag = std::clamp(lag, 0, 17);
}

animation::animation(c_cs_player* player, qangle_t last_reliable_angle) : animation(player){
	this->last_reliable_angle = last_reliable_angle;
}

void animation::restore(c_cs_player* player){
	player->get_velocity() = velocity;
	player->get_flags() = flags;
	player->get_eflags() = eflags;
	player->get_duck_amount() = duck;
	memcpy(player->get_animlayers(), layers, sizeof(anim_layer_t) * 13);
	player->get_lby() = lby;
	player->get_origin() = origin;
	player->set_abs_origin(origin);
}

void animation::apply(c_cs_player* player){
	player->set_pose_parameters(poses);
	player->get_eye_angles() = eye_angles;
	player->get_velocity() = player->get_abs_velocity() = velocity;
	player->get_lby() = lby;
	player->get_duck_amount() = duck;
	player->get_flags() = flags;
	player->get_origin() = origin;
	player->set_abs_origin(origin);
}

void animation::build_server_bones(c_cs_player* player){
	const auto backup_occlusion_flags = player->get_occlusion_flags();
	const auto backup_occlusion_framecount = player->get_occlusion_frame_count();

	player->get_occlusion_flags() = 0;
	player->get_occlusion_frame_count() = 0;

	player->get_bone_accessor()->m_readable_bones = player->get_bone_accessor()->m_writable_bones = 0;

	player->invalidate_bone_cache();

	player->get_effects().add(0x8);

	const auto backup_bone_array = player->get_bone_array_for_write();
	player->get_bone_array_for_write() = bones;

	globals::m_force_bone = true;
	player->setup_bones(nullptr, -1, 0x7FF00, sim_time);
	globals::m_force_bone = false;

	player->get_bone_array_for_write() = backup_bone_array;

	player->get_occlusion_flags() = backup_occlusion_flags;
	player->get_occlusion_frame_count() = backup_occlusion_framecount;

	player->get_effects().remove(~0x8);
}


void c_animations::update_players(){
	if (!interfaces::m_engine->is_in_game() && interfaces::m_engine->is_connected())
		return;

	const auto local_index = interfaces::m_engine->get_local_player();
	const auto local = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(local_index));

	if (!local || !local->is_alive())
		return;

	// erase outdated entries
	for (auto it = animation_infos.begin(); it != animation_infos.end();) {
		auto player = reinterpret_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity_from_handle(it->first));

		if (!player || player != it->second.player || !player->is_alive() || !local)
		{
			if (player)
				player->get_client_side_animation() = true;

			it = animation_infos.erase(it);
		}
		else
			it = next(it);
	}

	if (!local)
	{
		for (auto i = 1; i <= interfaces::m_global_vars->m_max_clients; ++i) {
			const auto entity = reinterpret_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));
			if (entity && entity->is_player())
				entity->get_client_side_animation() = true;
		}
	}

	for (auto i = 1; i <= interfaces::m_global_vars->m_max_clients; ++i) {
		const auto entity = reinterpret_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));

		if (!entity || !entity->is_player())
			continue;

		if (!entity->is_alive() || entity->is_dormant())
			continue;

		if (entity->get_index() == local->get_index())
			continue;

		if (!entity->is_enemy() && !entity->is_local_player())
			globals::m_call_client_update_enemy = entity->get_client_side_animation() = true;


		if (animation_infos.find(entity->get_handle().to_int()) == animation_infos.end())
			animation_infos.insert_or_assign(entity->get_handle().to_int(), animation_info(entity, { }));
	}

	// run post update
	for (auto& info : animation_infos)
	{
		auto& _animation = info.second;
		const auto player = _animation.player;

		// erase frames out-of-range
		for (auto i = _animation.frames.rbegin(); i != _animation.frames.rend();) {
			if (interfaces::m_global_vars->m_cur_time - i->sim_time > 1.2f)
				i = decltype(i) { info.second.frames.erase(next(i).base()) };
			else
				i = next(i);
		}

		// have we already seen this update?
		if (player->get_sim_time() == player->get_old_sim_time())
			continue;

		// reset animstate
		if (_animation.last_spawn_time != player->get_spawn_time())
		{
			const auto state = player->get_anim_state();
			if (state)
				player->reset_animation_state(state);

			_animation.last_spawn_time = player->get_spawn_time();
		}

		// grab weapon
		const auto weapon = player->get_active_weapon();

		auto backup = animation(player);
		backup.apply(player);

		// grab previous
		animation* previous = nullptr;

		if (!_animation.frames.empty() && !_animation.frames.front().dormant)
			previous = &_animation.frames.front();

		const auto shot = weapon && previous && weapon->get_last_shot_time() > previous->sim_time
			&& weapon->get_last_shot_time() <= player->get_sim_time();

		if (!shot)
			info.second.last_reliable_angle = player->get_eye_angles();

		// store server record
		auto& record = _animation.frames.emplace_front(player, info.second.last_reliable_angle);

		// run full update
		_animation.update_animations(&record, previous);

		backup.restore(player);

		// use uninterpolated data to generate our bone matrix
		record.build_server_bones(player);
	}
}

float animation::get_client_interp_amount() {
	return std::max(interfaces::m_cvar_system->find_var(FNV1A("cl_interp"))->get_float(),
		interfaces::m_cvar_system->find_var(FNV1A("cl_interp_ratio"))->get_float() / interfaces::m_cvar_system->find_var(FNV1A("cl_updaterate"))->get_float());
}

bool animation::is_valid(float m_sim_time, bool m_valid, float range/*, float max_unlag = .2f*/){
	if (!interfaces::m_client_state->m_net_channel || !m_valid)
		return false;

	static auto sv_maxunlag = interfaces::m_cvar_system->find_var(FNV1A("sv_maxunlag"));

	const auto correct = std::clamp(interfaces::m_client_state->m_net_channel->get_latency(FLOW_INCOMING) + interfaces::m_client_state->m_net_channel->get_latency(FLOW_OUTGOING)
		+ globals::hvh::m_lerp, 0.f, sv_maxunlag->get_float());

	return fabsf(correct - (interfaces::m_global_vars->m_cur_time - m_sim_time)) < range && correct < 1.f;
}

std::optional<animation*> c_animations::get_latest_animation(c_cs_player* player){
	const auto info = animation_infos.find(player->get_handle().to_int());
	if (info == animation_infos.end() || info->second.frames.empty()){
		return std::nullopt;
	}

	animation* result = nullptr;


	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it)) {
		if (!result)
			result = &*it;

		if (it->is_valid(it->sim_time, it->valid)) {
			return &*it;
		}
	}

	if (result)
		return result;
	else
		return std::nullopt;
}
std::optional<animation*> c_animations::get_oldest_animation(c_cs_player* player){
	const auto info = animation_infos.find(player->get_handle().to_int());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.rbegin(); it != info->second.frames.rend(); it = next(it)) {
		if (it->is_valid(it->sim_time, it->valid)) {
			return &*it;
		}
	}

	return std::nullopt;
}



std::optional<std::pair<animation*, animation*>> c_animations::get_valid_animations(c_cs_player* player, const float range){
	const auto info = animation_infos.find(player->get_handle().to_int());
	if (info == animation_infos.end() || info->second.frames.empty()) {
		return std::nullopt;
	}

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it)) {
		if (it->is_valid(it->sim_time, it->valid, range * .2f) && it + 1 != info->second.frames.end()
			&& !((it + 1)->is_valid((it + 1)->sim_time, (it + 1)->valid, range * .2f))) {
			return std::make_pair(&*(it + 1), &*it);
		}
	}

	return std::nullopt;
}
std::vector<animation*> c_animations::get_latest_firing_animation(c_cs_player* player, const float range){
	std::vector<animation*> result;

	const auto info = animation_infos.find(player->get_handle().to_int());
	if (info == animation_infos.end() || info->second.frames.empty())
		return result;

	result.reserve(static_cast<int>(std::ceil(range * .2f / interfaces::m_global_vars->m_interval_per_tick)));

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it)) {
		if (it->is_valid(it->sim_time, it->valid, range * .2f))
			result.push_back(&*it);
	}

	return result;
}