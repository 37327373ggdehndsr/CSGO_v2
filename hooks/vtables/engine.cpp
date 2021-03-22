#include "../hooks.h"

void __cdecl hooks::engine::cl_move::fn(float m1, bool m2) {
	static const auto original = m_engine->get_original<T>(index);
	
	if (!g_hvh->m_fake_duck){
		if (globals::m_local && globals::m_local->is_alive() && interfaces::m_engine->is_in_game() && interfaces::m_engine->is_connected() && tickbase->m_shift_data.m_should_attempt_shift && tickbase->m_shift_data.m_needs_recharge) {
			--tickbase->m_shift_data.m_needs_recharge;

			if (tickbase->m_shift_data.m_needs_recharge == 0) {
				tickbase->m_shift_data.m_should_be_ready = true;
			}

			return;
		}
	}
	
	original(m1, m2);
}

void __fastcall hooks::engine::fire_game_event::fn( void* ecx, void* edx ) {
	static const auto original = m_engine->get_original<T>(index);

	if ( !globals::m_local || !globals::m_local->is_alive( ) )
		return original(ecx, edx);

	auto event = *( event_info_t** ) ( uintptr_t( interfaces::m_client_state ) + 0x4E6C );
	while ( event ) {
		event->m_delay = 0.0f;
		event = event->m_next;
	}

	return original(ecx, edx);
}

void __fastcall hooks::engine::get_viewangles::fn( void* ecx, void* edx, vec3_t& ang ) {
	static const auto original = m_engine->get_original<T>(index);

	original(ecx, edx, ang);
}

bool __fastcall hooks::engine::is_connected::fn( void* ecx, void* edx ) {
	static const auto original = m_engine->get_original<T>(index);

	const auto ret = memory::stack_t().ret();

	static const memory::address_t IsLoadoutAllowed{ SIG("client.dll", "84 C0 75 04 B0 01 5F") };

	if (m_cfg.misc.unlock_inventory && ret == IsLoadoutAllowed)
		return false;

	return original(ecx, edx);
}

bool __fastcall hooks::engine::is_hltv::fn( void* ecx, void* edx ) {
	static const auto original = m_engine->get_original<T>(index);

	static auto return_to_setup_vel = ( void* ) SIG( "client.dll", "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" );
	static auto return_to_accumulate_layers = ( void* ) SIG( "client.dll", "84 C0 75 0D F6 87" );

	if (globals::m_call_bone)
		return true;

	if ( _ReturnAddress( ) == return_to_setup_vel || _ReturnAddress( ) == return_to_accumulate_layers )
		return true;

	return original(ecx, edx);
}

bool __fastcall hooks::engine::is_in_game::fn( void* ecx, void* edx ) {
	static const auto original = m_engine->get_original<T>(index);

	return original (ecx, edx);
}

bool __fastcall hooks::engine::is_paused::fn( void* ecx, void* edx ) {
	static const auto original = m_engine->get_original<T>(index);

	static auto return_to_extrapolation = SIG( "client.dll", "FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??" ).self_offset(0x29).cast<uintptr_t*>();

	if ( _ReturnAddress( ) == return_to_extrapolation )
		return true;

	return original(ecx, edx);
}