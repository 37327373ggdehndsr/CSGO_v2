#include "../features.h"

void extrapolate(c_cs_player* player, vec3_t& origin, vec3_t& velocity, bit_flag_t<uint32_t>& flags, bool on_ground) {
	vec3_t					  start, end, normal;
	c_game_trace			  trace;
	c_trace_filter_world_only filter;

	// define trace start.
	start = origin;

	// move trace end one tick into the future using predicted velocity.
	end = start + (velocity * interfaces::m_global_vars->m_interval_per_tick);

	// trace.
	interfaces::m_trace_system->trace_ray(ray_t(start, end, player->get_collideable()->obb_mins(), player->get_collideable()->obb_maxs()), CONTENTS_SOLID, &filter, &trace);

	// we hit shit
	// we need to fix shit.
	if (trace.m_fraction != 1.f) {

		// fix sliding on planes.
		for (int i{ }; i < 2; ++i) {
			velocity -= trace.m_plane.m_normal * velocity.dot_product(trace.m_plane.m_normal);

			float adjust = velocity.dot_product(trace.m_plane.m_normal);
			if (adjust < 0.f)
				velocity -= (trace.m_plane.m_normal * adjust);

			start = trace.m_end_pos;
			end = start + (velocity * (interfaces::m_global_vars->m_interval_per_tick * (1.f - trace.m_fraction)));

			interfaces::m_trace_system->trace_ray(ray_t(start, end, player->get_collideable()->obb_mins(), player->get_collideable()->obb_maxs()), CONTENTS_SOLID, &filter, &trace);
			if (trace.m_fraction == 1.f)
				break;
		}
	}

	// set new final origin.
	start = end = origin = trace.m_end_pos;

	// move endpos 2 units down.
	// this way we can check if we are in/on the ground.
	end.z -= 2.f;

	// trace.
	interfaces::m_trace_system->trace_ray(ray_t(start, end, player->get_collideable()->obb_mins(), player->get_collideable()->obb_maxs()), CONTENTS_SOLID, &filter, &trace);

	// strip onground flag.
	flags.remove(~FL_ONGROUND);

	// add back onground flag if we are onground.
	if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f)
		flags.add(FL_ONGROUND);
}

void c_animations::animation_info::update_animations( animation* record, animation* from ){
	if ( !from ){
		// set velocity and layers.
		record->velocity = player->get_velocity( );

		// fix feet spin.
		record->anim_state->m_feet_weight = 0.f;

		// apply record.
		record->apply( player );	

		// run update.
		return m_update_player( player );
	}

	const auto new_velocity = player->get_velocity( );

	// restore old record.
	player->set_abs_origin( record->origin );
	player->set_abs_angles( from->abs_ang );
	player->get_velocity( ) = from->velocity;

	// setup velocity.
	record->velocity = new_velocity;

	// setup extrapolation parameters.
	auto old_origin = from->origin;
	auto old_flags = from->flags;

	// did the player shoot?
	record->didshot = record->last_shot_time > from->sim_time && record->last_shot_time <= record->sim_time;

	for ( auto i = 0; i < record->lag; i++ ){
		// move time forward.
		const auto time = from->sim_time + TICKS_TO_TIME( i + 1 );
		const auto lerp = 1.f - ( record->sim_time - time ) / ( record->sim_time - from->sim_time );
		
		// lerp eye angles.
		auto eye_angles = math::interpolate(from->eye_angles, record->eye_angles, lerp);
		math::normalize(eye_angles);
		player->get_eye_angles() = eye_angles;

		// lerp duck amount.
		player->get_duck_amount() = math::interpolate(from->duck, record->duck, lerp);

		// resolve player.
		if ( record->lag - 1 == i ){
			player->get_velocity( ) = new_velocity;
			player->get_flags( ) = record->flags;
		}
		else{ // compute velocity and flags.

			extrapolate( player, old_origin, player->get_velocity( ), player->get_flags( ), old_flags.has(FL_ONGROUND));
			old_flags = player->get_flags( );
		}

		// correct shot desync.
		if (record->didshot){
			player->get_eye_angles() = record->last_reliable_angle;

			if (record->last_shot_time <= time)
				player->get_eye_angles() = record->eye_angles;
		}

		player->get_anim_state( )->m_feet_weight = 0.f;

		// backup simtime.
		const auto backup_simtime = player->get_sim_time( );

		// set new simtime.
		player->get_sim_time( ) = time;

		// run update enemy
		g_animations->manage_enemy_animations(player, record);

		// call resolver
	//	g_resolver->resolve_yaw(record);

		// run update.
		m_update_player( player );

		// restore old simtime.
		player->get_sim_time( ) = backup_simtime;
	}
}

float AngleDiff(float destAngle, float srcAngle) {
	float delta;

	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle) {
		if (delta >= 180)
			delta -= 360;
	}
	else {
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

void c_animations::manage_enemy_animations(c_cs_player* pEntity, animation* record) {
	memcpy(m_ResolverLayers, pEntity->get_animlayers(),  sizeof(anim_layer_t) * pEntity->m_anim_overlay());

	//local vars 
	float m_flEyeYaw = record->anim_state->m_eye_yaw;
	float m_flGoalFeetYaw = record->anim_state->m_foot_yaw;

	// check speed if <= 0.1 = lby
	if (record->player->get_velocity().length_2d() <= 0.1) {
		if (record->layers[3].m_weight == 0.0 && record->layers[3].m_cycle == 0.0 && record->layers[6].m_weight == 0.0) {
			BRUTE* (AngleDiff(m_flEyeYaw, m_flGoalFeetYaw) <= 0.0) - 1;
		}
	}
	// layer 12 = lean | layer 6 = movement
	else if (!(record->layers[12].m_weight * 1000.0) && int(record->layers[6].m_weight * 1000.0) == int(pEntity->get_animlayers()[6].m_weight * 1000.0)) {
		// moving side detect.
		unsigned int DeltaFirst = abs(record->layers[6].m_playback_rate - m_ResolverLayers[0][6].m_playback_rate);
		unsigned int DeltaSecond = abs(record->layers[6].m_playback_rate - m_ResolverLayers[1][6].m_playback_rate);
		unsigned int DeltaThird = abs(record->layers[6].m_playback_rate - m_ResolverLayers[2][6].m_playback_rate);

		if (DeltaFirst < DeltaSecond || DeltaThird <= DeltaSecond || (DeltaSecond * 1000.0)) {
			if (DeltaFirst >= DeltaThird && DeltaSecond > DeltaThird && !(DeltaThird * 1000.0)) {
				LEFT; // Left side = 1.
			}
		}
		else {
			RIGHT; // Right side = -1.
		}
	}
}

void c_animations::animation_info::m_update_player( c_cs_player* pEnt ){
	static bool& invalidate_bone_cache = **reinterpret_cast< bool** >( SIG( "client.dll", "C6 05 ? ? ? ? ? 89 47 70" ).get( ) + 2 );

	float curtime = interfaces::m_global_vars->m_cur_time;
	float frametime = interfaces::m_global_vars->m_frame_time;

	// get player anim state.
	c_anim_state* state = pEnt->get_anim_state();
	if (!state)
		return;

	float m_flSimulationTime = pEnt->get_sim_time( );
	int m_iNextSimulationTick = m_flSimulationTime / interfaces::m_global_vars->m_interval_per_tick + 1;

	// fixes for networked players
	interfaces::m_global_vars->m_frame_time = interfaces::m_global_vars->m_interval_per_tick;
	interfaces::m_global_vars->m_cur_time = pEnt->get_sim_time();
	pEnt->get_eflags().remove(~0x1000);
		
	if (state->m_last_frame_count == interfaces::m_global_vars->m_frame_count )
		state->m_last_frame_count -= 1.f;

	const bool backup_invalidate_bone_cache = invalidate_bone_cache;

	globals::m_call_client_update_enemy = true;
	pEnt->get_client_side_animation( ) = true;

	pEnt->update_client_side_animation( );
	
	pEnt->get_client_side_animation( ) = false;
	globals::m_call_client_update_enemy = false;

	pEnt->invalidate_physics_recursive( ANGLES_CHANGED );
	pEnt->invalidate_physics_recursive( ANIMATION_CHANGED );
	pEnt->invalidate_physics_recursive( SEQUENCE_CHANGED );

	invalidate_bone_cache = backup_invalidate_bone_cache;
	
	interfaces::m_global_vars->m_frame_time = frametime;
	interfaces::m_global_vars->m_cur_time = curtime;
}

void c_animations::manage_local_animations() { //v3 it's u ?
	// get player anim state.
	c_anim_state*  state = globals::m_local->get_anim_state();
	if (!state)
		return;

	// allow re-animating in the same frame.
	if (state->m_last_frame_count == interfaces::m_global_vars->m_frame_count) {
		state->m_last_frame_count -= 1.f;
	}

	if (globals::m_animate) {
		// get layers.
		globals::m_local->get_animate_layers(m_real_layers);

		// allow the game to update animations this tick.
		globals::m_call_client_update = true;

		// update animations.
		globals::m_local->update_animation_state(state, globals::angles::m_anim);
		globals::m_local->update_client_side_animation();

		// disallow the game from updating animations this tick.
		globals::m_call_client_update = false;

		// save data when our choke cycle resets.
		if (!interfaces::m_client_state->m_choked_commands) {
			globals::m_rotation.y = state->m_foot_yaw;
			globals::m_local->get_pose_parameters(m_real_poses);
		}

		// remove model sway.
		m_real_layers[12].m_weight = 0.f;

		// make sure to only animate once per tick.
		globals::m_animate = false;
	}

	// update our layers and poses with the saved ones.
	globals::m_local->set_animate_layers(m_real_layers);
	globals::m_local->set_pose_parameters(m_real_poses);

	// update our real rotation.
	globals::m_local->set_abs_angles(globals::m_rotation);

	// build bones.
	globals::m_call_bone = true;
	globals::m_local->setup_bones(real_matrix, 128, 0x7FF00, globals::m_local->get_sim_time());
	globals::m_call_bone = false;
}

void c_animations::manage_fake_animations() {
	static c_base_handle* selfhandle = nullptr;
	static float spawntime = globals::m_local->get_spawn_time();

	auto allocate = m_fake_state == nullptr;
	auto change = !allocate && selfhandle != &globals::m_local->get_handle();
	auto reset = !allocate && !change && globals::m_local->get_spawn_time() != spawntime;

	// player changed, free old animation state.
	if (change && m_fake_state_allocated){
		interfaces::m_mem_alloc->free(m_fake_state_allocated);

		m_fake_state_allocated = nullptr;
	}

	// need to reset? (on respawn)
	if (reset) {
		// reset animation state.
		if (m_fake_state){
			globals::m_local->reset_animation_state(m_fake_state);
		}

		// note new spawn time.
		spawntime = globals::m_local->get_spawn_time();
	}

	// need to allocate or create new due to player change.
	if (allocate || change) {
		// only works with games heap alloc.
		m_fake_state_allocated = (c_anim_state*)interfaces::m_mem_alloc->alloc(sizeof(c_anim_state));

		if (m_fake_state_allocated){
			globals::m_local->create_animation_state(m_fake_state_allocated);
		}

		// used to detect if we need to recreate / reset.
		selfhandle = (c_base_handle*)&globals::m_local->get_handle();
		spawntime = globals::m_local->get_spawn_time();

		// note anim state for future use.
		m_fake_state = m_fake_state_allocated;
	}
	else {
		// make sure our state isn't null.
		if (!m_fake_state)
			return;

		// allow re-animating in the same frame.
		if (m_fake_state->m_last_frame_count == interfaces::m_global_vars->m_frame_count) {
			m_fake_state->m_last_frame_count -= 1;
		}

		// update fake animations per tick.
		if (globals::m_update_fake) {
			// update fake animations when we send commands.
			if (!interfaces::m_client_state->m_choked_commands) {

				// update fake animation state.
				globals::m_local->update_animation_state(m_fake_state, globals::angles::m_anim);

				globals::m_local->get_animate_layers(m_fake_layers);
				globals::m_local->get_pose_parameters(m_fake_poses);
			}

			// remove model sway.
			m_fake_layers[12].m_weight = 0.f;

			// make sure to only animate once per tick.
			globals::m_update_fake = false;
		}

		// replace current animation layers and poses with the fake ones.
		globals::m_local->set_animate_layers(m_fake_layers);
		globals::m_local->set_pose_parameters(m_fake_poses);

		// replace the model rotation and build a matrix with our fake data.
		globals::m_local->set_abs_angles(qangle_t(0.f, m_fake_state->m_foot_yaw, 0.f));

		// generate a fake matrix.
		globals::m_call_bone = true;
		globals::m_local->setup_bones(fake_matrix, 128, 0x7FF00, globals::m_local->get_sim_time());
		globals::m_call_bone = false;

		// fix interpolated model.
		for (auto& i : fake_matrix) {
			i[0][3] -= globals::m_local->get_render_origin().x;
			i[1][3] -= globals::m_local->get_render_origin().y;
			i[2][3] -= globals::m_local->get_render_origin().z;
		}

		// revert our layers and poses back.
		globals::m_local->set_animate_layers(m_real_layers);
		globals::m_local->set_pose_parameters(m_real_poses);

		// replace the model rotation with the proper rotation.
		globals::m_local->set_abs_angles(qangle_t(0.f, globals::m_rotation.y, 0.f));
	}
}

c_animations::animation_info* c_animations::get_animation_info(c_cs_player* player){
	auto info = animation_infos.find(player->get_handle().to_int());

	if (info == animation_infos.end())
		return nullptr;

	return &info->second;
}
