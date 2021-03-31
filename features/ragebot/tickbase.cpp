#include "../features.h"

bool tick_base_system::CanFireWeapon(float curtime ){
	auto weapon = globals::m_local->get_active_weapon();
	if (!weapon)
		return false;

	bool can_fire = TICKS_TO_TIME( globals::m_local->get_tick_base( ) ) >= globals::m_local->get_next_attack( );

	if ( !can_fire )
		return false;

	if ( curtime >= weapon->get_next_primary_attack( ) )
		return true;

	return false;
}

bool tick_base_system::IsFiring( float curtime ){
	return globals::m_cmd->m_buttons.has( IN_ATTACK ) && CanFireWeapon(curtime );
}


bool tick_base_system::CanFireWeapon(float curtime) {
	// the player cant fire.
	if (!globals::hvh::m_player_fire || !globals::m_local->get_active_weapon())
		return false;

	if (globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_GRENADE)
		return false;

	// if we have no bullets, we cant shoot.
	if (globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type != WEAPON_TYPE_KNIFE && globals::m_local->get_active_weapon()->get_ammo() < 1)
		return false;

	// do we have any burst shots to handle?
	if ((globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_GLOCK18 || globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_FAMAS) && globals::m_local->get_active_weapon()->get_burst_shots_remaining() > 0) {
		// new burst shot is coming out.
		if (curtime >= globals::m_local->get_active_weapon()->get_next_burst_shot())
			return true;
	}

	// r8 revolver.
	if (globals::m_local->get_active_weapon()->get_item_definition_index() ==WEAPON_R8_REVOLVER) {
		// mouse1.
		if (globals::m_revolver_fire) {
			return true;
		}
		else {
			return false;
		}
	}

	// yeez we have a normal gun.
	if (curtime >= globals::m_local->get_active_weapon()->get_next_primary_attack())
		return true;

	return false;
}

bool tick_base_system::IsFiring(float curtime) {
	const auto weapon = globals::m_local->get_active_weapon();
	if (!weapon)
		return false;

	const auto IsZeus = globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_ZEUS_X27;
	const auto IsKnife = !IsZeus && globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_KNIFE;

	if (globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_GRENADE)
		return !weapon->get_pin_pulled() && weapon->get_throw_time() > 0.f && weapon->get_throw_time() < curtime;
	else if (IsKnife)
		return (globals::m_cmd->m_buttons.has(IN_ATTACK) || globals::m_cmd->m_buttons.has(IN_ATTACK2)) && CanFireWeapon(curtime);
	else
		return globals::m_cmd->m_buttons.has(IN_ATTACK) && CanFireWeapon(curtime);
}

void tick_base_system::WriteUserCmd( c_bf_write* buf, c_user_cmd* incmd, c_user_cmd* outcmd ){
	using WriteUsercmd_t = void( __fastcall* )( void*, c_user_cmd*, c_user_cmd* );
	static WriteUsercmd_t fnWriteUserCmd = SIG( "client.dll", "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ).cast<WriteUsercmd_t>( );

	__asm {
		mov     ecx, buf
		mov     edx, incmd
		push	outcmd
		call    fnWriteUserCmd
		add     esp, 4
	}
}

void tick_base_system::PreMovement( ){
	m_shift_data.m_next_shift_amount = m_shift_data.m_ticks_to_shift = 0;
}

void tick_base_system::PostMovement( ){
	if ( !m_cfg.ragebot.main.enabled )
		return;

	auto weapon = globals::m_local->get_active_weapon( );

	if ( !weapon || !globals::m_cmd ) {
		return;
	}

	auto data = weapon->get_cs_weapon_data( );

	if ( !data ) {
		return;
	}

	if ( weapon->get_item_definition_index( ) == WEAPON_R8_REVOLVER ||
		weapon->get_item_definition_index( ) == WEAPON_C4 ||
		data->m_weapon_type == WEAPON_TYPE_KNIFE ||
		data->m_weapon_type == WEAPON_TYPE_GRENADE )
	{
		m_shift_data.m_did_shift_before = false;
		m_shift_data.m_should_be_ready = false;
		m_shift_data.m_should_disable = true;
		return;
	}

	if ( !m_shift_data.m_should_attempt_shift ) {
		m_shift_data.m_did_shift_before = false;
		m_shift_data.m_should_be_ready = false;
		return;
	}

	m_shift_data.m_should_disable = false;
	
	bool bFastRecovery = false;
	float flNonShiftedTime = TICKS_TO_TIME( globals::m_local->get_tick_base( ) - globals::hvh::m_goal_shift );

	bool bCanShootNormally = CanFireWeapon(TICKS_TO_TIME( globals::m_local->get_tick_base( ) ) );
	bool bCanShootIn12Ticks = CanFireWeapon(flNonShiftedTime );
	bool bIsShooting = IsFiring(TICKS_TO_TIME( globals::m_local->get_tick_base( ) ) );

	m_shift_data.m_can_shift_tickbase = bCanShootIn12Ticks || ( !bCanShootNormally || bFastRecovery ) && ( m_shift_data.m_did_shift_before );

	if ( m_shift_data.m_can_shift_tickbase && !g_hvh->m_fake_duck) {
	
		m_shift_data.m_next_shift_amount = globals::hvh::m_goal_shift;
	}
	else {
		m_shift_data.m_next_shift_amount = 0;
		m_shift_data.m_should_be_ready = false;
	}

	if ( m_shift_data.m_next_shift_amount > 0 ) {
		
		if ( bCanShootIn12Ticks ) {
			if ( m_shift_data.m_prepare_recharge && !bIsShooting ) {
				m_shift_data.m_needs_recharge = globals::hvh::m_goal_shift;
				m_shift_data.m_prepare_recharge = false;
			}
			else {
				if ( bIsShooting ) {
					
					m_prediction.m_shifted_command = globals::m_cmd->m_command_number;
					m_prediction.m_shifted_ticks = abs( m_shift_data.m_current_shift );
					m_prediction.m_original_tickbase = globals::m_local->get_tick_base( );
					m_shift_data.m_ticks_to_shift = m_shift_data.m_next_shift_amount;
				}
			}
		}
		else {
			m_shift_data.m_prepare_recharge = true;
			m_shift_data.m_should_be_ready = false;
		}
	}
	else {
		m_shift_data.m_prepare_recharge = true;
		m_shift_data.m_should_be_ready = false;
	}

	m_shift_data.m_did_shift_before = m_shift_data.m_next_shift_amount > 0;
}