#include "../hooks.h"

void __fastcall hooks::predictions::run_cmd::fn(void* ecx, void*, c_cs_player* player, c_user_cmd* ucmd, i_move_helper* helper){
	static auto original = m_prediction->get_original<T>(index);

	if (!player || !player->is_alive() || player->get_index() != interfaces::m_engine->get_local_player())
		return original(ecx, player, ucmd, helper);

	if (ucmd->m_tick_count >= (globals::backup::m_tick + int(1 / interfaces::m_global_vars->m_interval_per_tick) + 8)) {
		ucmd->m_predicted = true;
		return;
	}

	int backup_tickbase = player->get_tick_base();
	float backup_curtime = interfaces::m_global_vars->m_cur_time;

	if (ucmd->m_command_number == tickbase->m_prediction.m_shifted_command) {
		player->get_tick_base() = (tickbase->m_prediction.m_original_tickbase - tickbase->m_prediction.m_shifted_ticks + 1);
		++player->get_tick_base();

		interfaces::m_global_vars->m_cur_time = TICKS_TO_TIME(player->get_tick_base());
	}

	float m_flVelModBackup = player->get_velocity_modifier();
	engine_prediction->patch_attack_packet(ucmd, true);
	if (globals::backup::m_override_velocity && ucmd->m_command_number == interfaces::m_client_state->m_last_command_ack + 1)
		player->get_velocity_modifier() = globals::backup::m_fl_velocity;


	original(ecx, player, ucmd, helper);

	if (!globals::backup::m_override_velocity)
		player->get_velocity_modifier() = m_flVelModBackup;

	if (ucmd->m_command_number == tickbase->m_prediction.m_shifted_command) {
		player->get_tick_base() = backup_tickbase;

		interfaces::m_global_vars->m_cur_time = backup_curtime;
	}

	engine_prediction->patch_attack_packet(ucmd, false);

	engine_prediction->update_viewmodel_data();
}

bool __fastcall hooks::predictions::in_prediction::fn(void* p){
	const auto ofunc = m_prediction->get_original<T>(index);

	static auto maintain_sequence_transitions = (void*)(SIG("client.dll", "84 C0 74 17 8B 87"));
	static auto setup_bones_ptr = (void*)(SIG("client.dll", "8B 40 ? FF D0 84 C0 74 ? F3 0F 10 05 ? ? ? ? EB ?") + 5);

	if (_ReturnAddress() == maintain_sequence_transitions || _ReturnAddress() == setup_bones_ptr)
		return false;

	return ofunc(p);
}