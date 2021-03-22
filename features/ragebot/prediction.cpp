#include "../features.h"

void c_engine_prediction::update() {
	if (interfaces::m_client_state->m_delta_tick <= 0)
		return;

	interfaces::m_prediction->update(
		interfaces::m_client_state->m_delta_tick, true,
		interfaces::m_client_state->m_last_command_ack, interfaces::m_client_state->m_last_outgoing_command + interfaces::m_client_state->m_choked_commands
	);
}

void c_engine_prediction::predict() {
	if (!globals::m_local->is_alive())
		return;

	m_player = globals::m_local;
	*m_random_seed = globals::m_cmd->m_random_seed;

	interfaces::m_prediction->m_in_prediction = true;
	interfaces::m_prediction->m_first_time_predicted = false;

	interfaces::m_global_vars->m_cur_time = TICKS_TO_TIME(globals::m_local->get_tick_base());
	interfaces::m_global_vars->m_frame_time = globals::m_local->get_flags().has(FL_FROZEN) ? 0.f : interfaces::m_global_vars->m_interval_per_tick;

	interfaces::m_move_helper->set_host(globals::m_local);

	interfaces::m_game_movement->start_track_prediction_errors(globals::m_local);

	interfaces::m_game_movement->process_movement(globals::m_local, m_move_data);

	interfaces::m_prediction->finish_move(globals::m_local, globals::m_cmd, m_move_data);

	interfaces::m_game_movement->finish_track_prediction_errors(globals::m_local);

	interfaces::m_move_helper->set_host(nullptr);

	m_player = nullptr;
	*m_random_seed = -1;

	const auto weapon = globals::m_local->get_active_weapon();
	if (!weapon) {
		m_spread = m_inaccuracy = 0.f;
		return;
	}

	weapon->update_accuracy();

	m_spread = weapon->get_spread();

	m_inaccuracy = weapon->get_inaccuracy();
}

void c_engine_prediction::correct_viewmodel_data() {
	if (globals::m_local->get_view_model() != 0xFFFFFFFF) {
		const auto view_model = (c_cs_player*)interfaces::m_entity_list->get_client_entity_from_handle(globals::m_local->get_view_model());
		if (!view_model)
			return;

		view_model->get_animtime() = stored_viewmodel.m_viewmodel_anim_time;
		view_model->get_cycle() = stored_viewmodel.m_viewmodel_cycle;
	}
}

void c_engine_prediction::update_viewmodel_data() {
	if (globals::m_local->get_view_model() != 0xFFFFFFFF) {
		const auto view_model = (c_cs_player*)interfaces::m_entity_list->get_client_entity_from_handle(globals::m_local->get_view_model());
		if (!view_model)
			return;

		stored_viewmodel.m_viewmodel_anim_time = view_model->get_animtime();
		stored_viewmodel.m_viewmodel_cycle = view_model->get_cycle();
	}
}

void c_engine_prediction::patch_attack_packet(c_user_cmd* cmd, bool predicted) {
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;

	if (predicted) {
		m_bLastAttack = cmd->m_weapon_select || (cmd->m_buttons.has(IN_ATTACK));
		m_flLastCycle = globals::m_local->get_cycle();
	}
	else if (m_bLastAttack && !m_bInvalidCycle)
		m_bInvalidCycle = globals::m_local->get_cycle() == 0.f && m_flLastCycle > 0.f;

	if (m_bInvalidCycle)
		globals::m_local->get_cycle() = m_flLastCycle;
}

void c_engine_prediction::update_velocity(){
	static int m_iLastCmdAck = 0;
	static float m_flNextCmdTime = 0.f;

	if (interfaces::m_client_state && (m_iLastCmdAck != interfaces::m_client_state->m_last_command_ack || m_flNextCmdTime != interfaces::m_client_state->m_next_cmd_time)){
		if (globals::backup::m_fl_velocity != globals::m_local->get_velocity_modifier()){
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(interfaces::m_prediction + 0x24)) = 1;
			globals::backup::m_fl_velocity = globals::m_local->get_velocity_modifier();
		}

		m_iLastCmdAck = interfaces::m_client_state->m_last_command_ack;
		m_flNextCmdTime = interfaces::m_client_state->m_next_cmd_time;
	}
}

void c_engine_prediction::process() {
	m_backup.store();

	m_player = globals::m_local;
	*m_random_seed = globals::m_cmd->m_random_seed;

	interfaces::m_global_vars->m_cur_time = TICKS_TO_TIME(globals::m_local->get_tick_base());
	interfaces::m_global_vars->m_frame_time = interfaces::m_global_vars->m_interval_per_tick;

	interfaces::m_game_movement->start_track_prediction_errors(globals::m_local);

	interfaces::m_prediction->setup_move(globals::m_local, globals::m_cmd, interfaces::m_move_helper, m_move_data);

	predict();
}

void c_engine_prediction::restore() {
	m_player = nullptr;
	*m_random_seed = -1;

	m_backup.restore();
}
