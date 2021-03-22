#include "../hooks.h"

bool __stdcall hooks::client_mode::create_move::fn(int sequence_number, c_user_cmd* cmd) {
	static const auto original = m_client_mode->get_original<T>(index);
	memory::stack_t   stack;

	if (!globals::m_local ||!cmd || !cmd->m_command_number)
		return original(sequence_number, cmd);
	
	globals::m_cmd = cmd;
	globals::m_packet = true;

	if (globals::m_local->is_alive() && interfaces::m_engine->is_connected()) {
		globals::backup::m_tick = cmd->m_tick_count;
		globals::hvh::m_lag = interfaces::m_client_state->m_choked_commands;

		globals::hvh::m_pressing_move = (cmd->m_buttons.has(IN_LEFT) || cmd->m_buttons.has(IN_FORWARD) || cmd->m_buttons.has(IN_BACK) ||
			cmd->m_buttons.has(IN_RIGHT) || cmd->m_buttons.has(IN_MOVELEFT) || cmd->m_buttons.has(IN_MOVERIGHT) ||
			cmd->m_buttons.has(IN_JUMP));

		if (m_cfg.ragebot.main.enabled)
			tickbase->PreMovement();
		globals::backup::m_override_velocity = true;

		movement->set_view_angles(cmd->m_view_angles);

		g_hvh->run();
		g_hvh->sendpacket();

		engine_prediction->update();
		movement->on_create_move(false);
		engine_prediction->process(); {
			globals::m_shot = false;


			if (tickbase->IsFiring(interfaces::m_global_vars->m_cur_time)) {
				globals::m_shot_command_number = cmd->m_command_number;

				if (!g_hvh->m_fake_duck) {
					globals::m_packet = true;
				}

				if (!globals::m_shot)
					globals::m_shot = true;
			}

			if (globals::m_packet) {
				g_hvh->m_step_switch = (bool)interfaces::random_int(0, 1);

				vec3_t cur = globals::m_local->get_origin();

				vec3_t prev = globals::m_net_pos.empty() ? cur : globals::m_net_pos.front().m_pos;

				globals::m_net_pos.emplace_front(interfaces::m_global_vars->m_cur_time, cur);
			}
		}engine_prediction->restore();

		cmd->m_view_angles.normalize();

		movement->on_create_move(true);

		if (globals::m_packet) {
			globals::angles::m_anim = cmd->m_view_angles;
		}
		else {
			globals::angles::m_anim = cmd->m_view_angles;
		}

		globals::backup::m_override_velocity = false;

		if (m_cfg.ragebot.main.enabled)
			tickbase->PostMovement();
	}

	globals::m_animate = true;
	globals::m_update_fake = true;
	
	*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) + 0x14) = globals::m_packet;

	return false;
}

void __stdcall hooks::client_mode::override_view::fn(view_setup_t* view) {
	static const auto original = m_client_mode->get_original<T>(index);

	original(view);
}