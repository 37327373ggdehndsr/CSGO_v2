#include "../features.h"

void hvh::run() {
	if (!m_cfg.hvh.enabled)
		return;

	if (!globals::m_local->is_alive())
		return;

	if ((globals::m_local->get_move_type() == MOVE_TYPE_NOCLIP ||
		globals::m_local->get_move_type() == MOVE_TYPE_LADDER) && globals::hvh::m_pressing_move) {
		return;
	}

	auto weapon = globals::m_local->get_active_weapon();
	if (!weapon)
		return;

	// are we throwing a nade?
	// if so then fuck off don't do anti aim
	if (weapon->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_GRENADE)
		if (weapon->get_throw_time() > 0)
			return;

	if (globals::m_cmd->m_buttons.has(IN_USE))
		return;

	if (tickbase->IsFiring(TICKS_TO_TIME(globals::m_local->get_tick_base())))
		return;

	globals::hvh::m_negate_desync = m_cfg.hvh.inver_key;

	pitch();
	yaw();
//	at_target();
	adjustyaw();
	predict_lby();
	//slowwalk();
	//fakeduck();
}

void hvh::pitch() {
	switch (m_cfg.hvh.pitch) {
	case 1:
		// down.
		globals::m_cmd->m_view_angles.x = 89.f;
		break;
	case 2:
		// up.
		globals::m_cmd->m_view_angles.x = -89.f;
		break;
	}
}

void hvh::yaw() {
	switch (m_cfg.hvh.yaw) {
		// backwards.
	case 1:
		globals::m_cmd->m_view_angles.y = 179.f;
		break;
		// random.
	case 2:
		// check update time.
		if (interfaces::m_global_vars->m_cur_time >= m_next_random_update) {

			// set new random angle.
			m_random_angle = interfaces::random_float(-180.f, 180.f);

			// set next update time
			m_next_random_update = interfaces::m_global_vars->m_cur_time + 0.4f;
		}

		// apply angle.
		globals::m_cmd->m_view_angles.y = m_random_angle;
		break;
	}

	if (m_cfg.hvh.jitter_type > 0) {
		if (globals::m_packet)
			m_jitter_update = !m_jitter_update;

		int range = m_cfg.hvh.jitter_range / 2.f;

		// offset.
		if (m_cfg.hvh.jitter_type == 1) {
			globals::m_cmd->m_view_angles.y += m_jitter_update ? -range : range;
		}
		// random.
		else {
			globals::m_cmd->m_view_angles.y += m_random_angle = interfaces::random_float(-60, 60);
		}
	}

	if (m_cfg.hvh.enabled_around_fake_jitter){
		// stand.
		if (m_cfg.hvh.real_around_fake_moving && globals::m_local->get_velocity().length() < 0.1f && !m_slow_walk) {
			m_use_real_around_fake = true;
		}
		// move.
		else if (m_cfg.hvh.real_around_fake_moving && globals::m_local->get_velocity().length() > 0.1f && !m_slow_walk) {
			m_use_real_around_fake = true;
		}
		// air.
		else if (m_cfg.hvh.real_around_fake_air && !m_slow_walk && ((globals::m_cmd->m_buttons.has(IN_JUMP)) || !(globals::m_local->get_flags().has(FL_ONGROUND)))) {
			m_use_real_around_fake = true;
		}
		// move.
		else if (m_cfg.hvh.real_around_fake_slow_motion && globals::m_local->get_velocity().length() > 0.1f && m_slow_walk) {
			m_use_real_around_fake = true;
		}

		if (m_use_real_around_fake){
			float yaw = 120.f;
			if (m_cfg.hvh.enabled_around_fake_jitter) {
				if (globals::m_local) {
					yaw = m_jitter_update ? -yaw : yaw;
				}
			}

			globals::m_cmd->m_view_angles.y += globals::hvh::m_negate_desync ? -yaw : yaw;
		}
		else{
			m_use_real_around_fake = false;
		}
	}
}

void hvh::adjustyaw() {
	if (!m_cfg.hvh.fake)
		return;

	int custom_fake = m_cfg.hvh.max_fake_delta * 2;

	// fake around real.
	if (!m_use_real_around_fake) {
		// do real anti-aim.
		if (globals::m_packet) {
			globals::m_cmd->m_view_angles.y += custom_fake * globals::hvh::m_negate_desync;
		}
		// do fake anti-aim.
		else {
			globals::m_cmd->m_view_angles.y += custom_fake * globals::hvh::m_negate_desync;
		}
	}
	// real around fake.
	else {
		if (globals::m_packet) {
			globals::m_cmd->m_view_angles.y += custom_fake;
		}
		else {
			globals::m_cmd->m_view_angles.y += custom_fake;
		}
	}

}

void hvh::at_target() {
	if (!m_cfg.hvh.base_angle)
		return;

	float		 best_fov{ std::numeric_limits< float >::max() };
	float		 best_dist{ std::numeric_limits< float >::max() };
	float		 fov, dist;
	c_cs_player* target, * best_target{ nullptr };

	for (int i{ 1 }; i <= interfaces::m_global_vars->m_max_clients; ++i) {
		target = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));

		if (target->is_dormant())
			continue;

		// 'away crosshair'.
		if (m_cfg.hvh.base_angle == 1) {

			// check if a player was closer to our crosshair.
			fov = math::get_fov(globals::m_cmd->m_view_angles, globals::m_local->get_eye_pos(), target->world_space_center());
			if (fov < best_fov) {
				best_fov = fov;
				best_target = target;
			}
		}

		// 'away distance'.
		else if (m_cfg.hvh.base_angle == 2) {

			// check if a player was closer to us.
			dist = (target->get_origin() - globals::m_local->get_origin()).length_sqr();
			if (dist < best_dist) {
				best_dist = dist;
				best_target = target;
			}
		}
	}

	if (best_target) {
		// todo - dex; calculate only the yaw needed for this (if we're not going to use the x component that is).
		qangle_t angle;
		math::vector_angles(best_target->get_origin() - globals::m_local->get_origin(), angle);
		globals::m_cmd->m_view_angles.y = angle.y;
	}
}

void hvh::sendpacket() {
	// fake-lag enabled.
	if (m_cfg.hvh.fake_lag_enabled && globals::m_local->is_alive() && !(globals::m_local->get_flags().has(FL_FROZEN))) {
		// limit of lag.
		int limit = std::clamp<int>((int)m_cfg.hvh.fake_lag_limit, 2, 14);
		int mode = m_cfg.hvh.fake_lag_mode;

		// reset this here everytime..
		globals::hvh::m_should_lag = false;

		// get current origin.
		vec3_t cur = globals::m_local->get_origin();

		// get prevoius origin.
		vec3_t prev = globals::m_net_pos.empty() ? globals::m_local->get_origin() : globals::m_net_pos.front().m_pos;

		// delta between the current origin and the last sent origin.
		float delta = (cur - prev).length_sqr();

		// stand.
		if (delta < 0.1) {
			globals::hvh::m_should_lag = true;

			if (!m_cfg.hvh.fake_lag_on_stand)
				limit = 2;
		}

		// move.
		if (m_cfg.hvh.fake_lag_on_moving && delta > 0.1f && globals::m_local->get_velocity().length() > 0.1f && !m_fake_duck) {
			globals::hvh::m_should_lag = true;
		}

		// air.
		else if (m_cfg.hvh.fake_lag_on_air && !m_fake_duck && ((globals::m_cmd->m_buttons.has(IN_JUMP)) || !(globals::m_local->get_flags().has(FL_ONGROUND)))) {
			globals::hvh::m_should_lag = true;
		}

		// fake duck.
		else if (m_fake_duck) {
			globals::hvh::m_should_lag = true;
			limit = 14;
			mode = 0;
		}

		// commenting in gives the 'p2c effect' where it turns on fakelag between shots, though cba adjusting the current recharging..
		if (tickbase->m_shift_data.m_should_attempt_shift && ((!tickbase->m_shift_data.m_should_be_ready && tickbase->m_shift_data.m_prepare_recharge) || tickbase->m_shift_data.m_needs_recharge || tickbase->m_shift_data.m_should_be_ready) && !m_fake_duck) {
			globals::hvh::m_should_lag = true;
			limit = 2;
			mode = 0;
		}

		// talking.
		if (interfaces::m_engine->is_voice_recording()) {
			globals::hvh::m_should_lag = true;
			limit = 2;
			mode = 0;
		}

		globals::hvh::m_wanted_choke = limit;

		if (globals::hvh::m_should_lag) {
			// max.
			if (mode == 0)
				globals::m_packet = false;

			// break.
			else if (mode == 1 && delta <= 4096.f)
				globals::m_packet = false;

			// random.
			else if (mode == 2) {
				// compute new factor.
				if (globals::hvh::m_lag >= m_random_lag)
					m_random_lag = interfaces::random_int(2, limit);

				// factor not met, keep choking. 
				else globals::m_packet = false;
			}

			// break step.
			else if (mode == 3) {
				// normal break.
				if (m_step_switch) {
					if (delta <= 4096.f)
						globals::m_packet = false;
				}

				// max.
				else globals::m_packet = false;
			}

			if (globals::hvh::m_lag >= limit) {
				globals::m_packet = true;
			}
		}
	}
	else {
		globals::hvh::m_should_lag = false;
		globals::hvh::m_wanted_choke = 0;
	}

	if (m_cfg.hvh.fake_lag_on_peek) {
		static const auto		  sv_gravity = interfaces::m_cvar_system->find_var(FNV1A("sv_gravity"));
		vec3_t					  start = globals::m_local->get_origin(), end = start, vel = globals::m_local->get_velocity();
		c_trace_filter_world_only filter;
		c_game_trace			  trace;

		// gravity.
		vel.z -= (sv_gravity->get_float() * interfaces::m_global_vars->m_interval_per_tick);

		// extrapolate.
		end += (vel * interfaces::m_global_vars->m_interval_per_tick);

		// move down.
		end.z -= 2.f;

		interfaces::m_trace_system->trace_ray(ray_t(start, end), MASK_SOLID, &filter, &trace);

		// check if landed.
		if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f && !(globals::m_local->get_flags().has(FL_ONGROUND)))
			globals::m_packet = true;
	}

	// do not lag while shooting.
	if (globals::m_old_shot)
		globals::m_packet = true;


	// we somehow reached the maximum amount of lag.
	// we cannot lag anymore and we also cannot shoot anymore since we cant silent aim.
	if (globals::hvh::m_lag >= 14) {
		// set bSendPacket to true.
		globals::m_packet = true;
	}
}

void hvh::predict_lby() {
	if (!(globals::m_local->get_flags().has(FL_ONGROUND)))
		return;

	static float spawn_time = globals::m_local->get_spawn_time();
	if (globals::m_local->get_spawn_time() != spawn_time) {
		spawn_time = globals::m_local->get_spawn_time();
		m_lby_update_time = spawn_time;
		m_should_resync = true;
	}

	/* get our lby timing */
	if (globals::m_local->get_velocity().length_2d() > 0.1f) {
		m_lby_update_time = interfaces::m_global_vars->m_cur_time + 0.22f;
	}
	if (m_should_resync || m_lby_update_time < interfaces::m_global_vars->m_cur_time) {
		m_lby_update_time = interfaces::m_global_vars->m_cur_time + 1.1f;
		m_in_lby_update = true;
	}

	/* see if we are in balance update */
	if (m_lby_update_time - interfaces::m_global_vars->m_interval_per_tick < interfaces::m_global_vars->m_cur_time)
		m_in_balance_update = true;

	/* check if we can micromove so ur lby will update faster */
	if (interfaces::m_client_state->m_choked_commands <= 0 && !m_in_lby_update) { // micromove and add our lby delta to our desync
		m_can_micro_move = true;
	}
}

void hvh::slowwalk(){
	m_slow_walk = false;
	if (!m_cfg.hvh.slowwalk_key)
		return;
	
	m_slow_walk = true;
	auto weapon_auto = globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_G3SG1 && WEAPON_SCAR20;
	auto weapon_awp = globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_AWP;
	auto weapon_ssg = globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_SSG_08;

	if (weapon_auto) {
		if (globals::m_local->get_flags().has(FL_ONGROUND)) {
			movement->minwalk(40);
		}
	}
	if (weapon_awp) {
		if (globals::m_local->get_flags().has(FL_ONGROUND)) {
			movement->minwalk(33);
		}
	}
	if (weapon_ssg) {
		if (globals::m_local->get_flags().has(FL_ONGROUND)) {
			movement->minwalk(70);
		}
	}
	if (!weapon_awp && !weapon_auto && !weapon_ssg) {
		if (globals::m_local->get_flags().has(FL_ONGROUND)) {
			movement->minwalk(34);
		}
	}
}

void hvh::fakeduck() {
	globals::m_cmd->m_buttons.add(IN_BULLRUSH);

	m_fake_duck = false;
	if (!m_cfg.hvh.fakeduck_key)
		return;

	m_fake_duck = true;
	if (interfaces::m_client_state->m_choked_commands <= 7) {
		globals::m_cmd->m_buttons.remove(~IN_DUCK);
	}
	else {
		globals::m_cmd->m_buttons.add(IN_DUCK);
	}
}
