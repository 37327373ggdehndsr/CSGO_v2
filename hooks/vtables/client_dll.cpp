#include "../hooks.h"

void __stdcall hooks::client_dll::frame_stage_notify::fn(e_client_frame_stage stage) {
	static const auto original = m_client_dll->get_original<T>(index);
	
	if (interfaces::m_engine->is_in_game() && interfaces::m_engine->is_connected() && globals::m_local){
		globals::m_cur_stage = stage;

		if (stage == FRAME_RENDER_START ){
			if (globals::m_local->is_alive() && interfaces::m_input->m_camera_in_third_person)
				interfaces::m_prediction->set_local_view_angles(globals::angles::m_anim);
			
			globals::hvh::m_rate = (int)std::round(1.f / interfaces::m_global_vars->m_interval_per_tick);
		
			/* pvs fix */
			if (globals::m_local->is_alive()) {
				for (int i = 1; i < 65; i++) {
					auto player = c_cs_player::get_player_by_index(i);
					if (!player || !player->is_player() || player == globals::m_local) continue;

					*(int*)((uintptr_t)player + 0xA30) = interfaces::m_global_vars->m_frame_count;
					*(int*)((uintptr_t)player + 0xA28) = 0;
				}
			} 

			if (globals::m_local->is_alive()) {
				if (globals::m_local->get_flags() & FL_ONGROUND) {
					globals::m_local->get_anim_state()->m_on_ground = true;
				}

				globals::m_local->set_abs_angles(qangle_t(0, globals::m_rotation.y, 0));
			}
		}

		if (globals::m_local->is_alive()){
			int framstage_minus2 = stage - 2;

			if (framstage_minus2) {
				// wtf
			}
			else {
				engine_prediction->update_velocity();
			}
		}

		if (stage == FRAME_NET_UPDATE_END && globals::m_local->is_alive()){
			//static auto r_jiggle_bones = interfaces::m_cvar_system->find_var(FNV1A("r_jiggle_bones"));
			//*(float*)((DWORD)&r_jiggle_bones->m_callback + 0xC) = NULL;
			//if (r_jiggle_bones->get_int() > 0)
			//	r_jiggle_bones->set_value(0);

			for (int i = 1; i < interfaces::m_global_vars->m_max_clients; i++){
				auto entity = reinterpret_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));
				if (entity != nullptr && entity->is_player() && entity->get_index() != globals::m_local->get_index() && entity->is_alive()){
					const auto var_map = reinterpret_cast<uintptr_t>(entity) + 36;

					for (auto index = 0; index < *reinterpret_cast<int*>(var_map + 20); index++)
						*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(var_map) + index * 12) = 0;
				}
			}	

			g_animations->update_players();
			g_animations->manage_local_animations();
			g_animations->manage_fake_animations();
		}

		if (globals::m_local->is_alive() && stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START){
			static float m_curtime = interfaces::m_global_vars->m_cur_time + 0.1f;
			static bool should_update = false;

			engine_prediction->correct_viewmodel_data();

			if (globals::m_call_update) {
				globals::m_call_update = false;
				
				m_curtime = interfaces::m_global_vars->m_cur_time + 0.1f;
				
				should_update = true;
			}
			if (should_update) {
				if (interfaces::m_global_vars->m_cur_time > m_curtime) {
					should_update = false;

					typedef void(*ForceUpdate) (void);
					ForceUpdate FullUpdate = (ForceUpdate)SIG("engine.dll", "A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85").get();
					FullUpdate();
				}
			}
		}
	}

	original(stage);
}

bool __stdcall hooks::client_dll::write_user_cmd::fn(int m_nSlot, void* m_pBuffer, int m_nFrom, int m_nTo, bool m_bNewCmd) {
	static auto original = m_client_dll->get_original<T>(index);

	if (!m_cfg.ragebot.main.enabled || tickbase->m_shift_data.m_ticks_to_shift <= 0)
		return original(m_nSlot, m_pBuffer, m_nFrom, m_nTo, true);

	if (m_nFrom != -1)
		return true;

	m_nFrom = -1;

	int m_nTickbase = tickbase->m_shift_data.m_ticks_to_shift;
	tickbase->m_shift_data.m_ticks_to_shift = 0;

	int* m_pnNewCmds = (int*)((uintptr_t)m_pBuffer - 0x2C);
	int* m_pnBackupCmds = (int*)((uintptr_t)m_pBuffer - 0x30);

	*m_pnBackupCmds = 0;

	int m_nNewCmds = *m_pnNewCmds;
	int m_nNextCmd = interfaces::m_client_state->m_choked_commands + interfaces::m_client_state->m_last_outgoing_command + 1;
	int m_nTotalNewCmds = std::min(m_nNewCmds + abs(m_nTickbase), 16);

	*m_pnNewCmds = m_nTotalNewCmds;

	for (m_nTo = m_nNextCmd - m_nNewCmds + 1; m_nTo <= m_nNextCmd; m_nTo++) {
		if (!original(m_nSlot, m_pBuffer, m_nFrom, m_nTo, true))
			return false;

		m_nFrom = m_nTo;
	}

	c_user_cmd* m_pCmd = interfaces::m_input->get_user_cmd(m_nSlot, m_nFrom);
	if (!m_pCmd)
		return true;

	c_user_cmd m_ToCmd = *m_pCmd, m_FromCmd = *m_pCmd;
	m_ToCmd.m_command_number++;
	m_ToCmd.m_tick_count += 3 * globals::hvh::m_rate;

	for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++) {
		tickbase->WriteUserCmd((c_bf_write*)m_pBuffer, &m_ToCmd, &m_FromCmd);
		m_FromCmd = m_ToCmd;

		m_ToCmd.m_command_number++;
		m_ToCmd.m_tick_count++;
	}

	tickbase->m_shift_data.m_current_shift = m_nTickbase;
	return true;
}
