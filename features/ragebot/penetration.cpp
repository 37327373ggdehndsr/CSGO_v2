#include "../features.h"

ReturnInfo_t penetration::think( vec3_t pos, c_cs_player* target, int specific_hitgroup ){
	ReturnInfo_t return_info = ReturnInfo_t( -1, -1, 4, false, 0.f, nullptr );

	vec3_t start = globals::m_local->get_eye_pos( );

	FireBulletData_t fire_bullet_data;
	fire_bullet_data.m_start = start;
	fire_bullet_data.m_end = pos;
	fire_bullet_data.m_current_position = start;
	fire_bullet_data.m_thickness = 0.f;
	fire_bullet_data.m_penetration_count = 5;

	math::angle_vectors( math::calc_angle( start, pos ), fire_bullet_data.m_direction);

	static const auto filter_simple = *reinterpret_cast< uint32_t* >( reinterpret_cast< uint32_t >(
		( void* )SIG( "client.dll", "55 8B EC 83 E4 F0 83 EC 7C 56 52" ).get( ) ) + 0x3d );

	uint32_t dwFilter[ 4 ] = { filter_simple,
		reinterpret_cast< uint32_t >( ( c_cs_player * )globals::m_local ), 0, 0 };

	fire_bullet_data.m_filter = ( c_trace_filter* )( dwFilter );

	auto weapon = globals::m_local->get_active_weapon( );
	if ( !weapon ) return return_info;

	auto weapon_info = weapon->get_cs_weapon_data( );
	if ( !weapon_info ) return return_info;
	
	float range = std::min( weapon_info->m_range, ( start - pos ).length( ) );

	pos = start + ( fire_bullet_data.m_direction * range );
	fire_bullet_data.m_current_damage = weapon_info->m_damage;

	for ( int i = 0; i < scanned_points.size( ); i++ ) {
		if ( pos == scanned_points[ i ] ) {
			return_info.m_damage = scanned_damage[ i ];
			return return_info;
		}
	}

	while ( fire_bullet_data.m_current_damage > 0 && fire_bullet_data.m_penetration_count > 0 ){
		return_info.m_penetration_count = fire_bullet_data.m_penetration_count;

		trace_line( fire_bullet_data.m_current_position, pos, MASK_SHOT | CONTENTS_GRATE, globals::m_local, &fire_bullet_data.m_enter_trace );
		clip_trace( fire_bullet_data.m_current_position, fire_bullet_data.m_current_position + ( fire_bullet_data.m_direction * 40.f ), target, MASK_SHOT | CONTENTS_GRATE, fire_bullet_data.m_filter, &fire_bullet_data.m_enter_trace );

		const float distance_traced = ( fire_bullet_data.m_enter_trace.m_end_pos - start ).length( );
		fire_bullet_data.m_current_damage *= pow( weapon_info->m_range_modifier, ( distance_traced / 500.f ) );

		if ( fire_bullet_data.m_enter_trace.m_fraction == 1.f ){
			if ( target && specific_hitgroup != -1 ){
				scale_damage( target, weapon_info, specific_hitgroup, fire_bullet_data.m_current_damage );

				return_info.m_damage = fire_bullet_data.m_current_damage;
				return_info.m_hitgroup = specific_hitgroup;
				return_info.m_end = fire_bullet_data.m_enter_trace.m_end_pos;
				return_info.m_hit_entity = target;
			}
			else{
				return_info.m_damage = fire_bullet_data.m_current_damage;
				return_info.m_hitgroup = -1;
				return_info.m_end = fire_bullet_data.m_enter_trace.m_end_pos;
				return_info.m_hit_entity = nullptr;
			}

			break;
		}

		if ( fire_bullet_data.m_enter_trace.m_hitgroup > 0 && fire_bullet_data.m_enter_trace.m_hitgroup <= 8 )
		{
			if(( target && fire_bullet_data.m_enter_trace.m_hit_entity != target )||( reinterpret_cast< c_cs_player* >( fire_bullet_data.m_enter_trace.m_hit_entity )->get_team( ) == globals::m_local->get_team( ) ) ){
				return_info.m_damage = -1;
				return return_info;
			}

			if ( specific_hitgroup != -1 ){
				scale_damage( reinterpret_cast< c_cs_player* >( fire_bullet_data.m_enter_trace.m_hit_entity ), weapon_info, specific_hitgroup, fire_bullet_data.m_current_damage );
			}
			else{
				scale_damage( reinterpret_cast< c_cs_player* >( fire_bullet_data.m_enter_trace.m_hit_entity ), weapon_info, fire_bullet_data.m_enter_trace.m_hitgroup, fire_bullet_data.m_current_damage );
			}

			return_info.m_damage = fire_bullet_data.m_current_damage;
			return_info.m_hitgroup = fire_bullet_data.m_enter_trace.m_hitgroup;
			return_info.m_end = fire_bullet_data.m_enter_trace.m_end_pos;
			return_info.m_hit_entity = ( c_cs_player* )fire_bullet_data.m_enter_trace.m_hit_entity;

			break;
		}

		if ( !handle_bullet_penetration( weapon_info, fire_bullet_data ) )
			break;

		return_info.m_did_penetrate_wall = true;
	}

	scanned_damage.push_back( return_info.m_damage );
	scanned_points.push_back( pos );

	return_info.m_penetration_count = fire_bullet_data.m_penetration_count;

	return return_info;
}

float penetration::hitgroup_damage( int iHitGroup ){
	switch ( iHitGroup ){
	case HITGROUP_GENERIC:
		return 0.5f;
	case HITGROUP_HEAD:
		return 2.0f;
	case HITGROUP_CHEST:
		return 0.5f;
	case HITGROUP_STOMACH:
		return 0.75f;
	case HITGROUP_LEFT_ARM:
		return 0.5f;
	case HITGROUP_RIGHT_ARM:
		return 0.5f;
	case HITGROUP_LEFT_LEG:
		return 0.375f;
	case HITGROUP_RIGHT_LEG:
		return 0.375f;
	case HITGROUP_GEAR:
		return 0.5f;
	default:
		return 1.0f;
	}

	return 1.0f;
}

void penetration::scale_damage( c_cs_player* e, c_cs_weapon_data* weapon_info, int hitgroup, float& current_damage ){
	static auto mp_damage_scale_ct_head = interfaces::m_cvar_system->find_var(FNV1A( "mp_damage_scale_ct_head" ) );
	static auto mp_damage_scale_t_head = interfaces::m_cvar_system->find_var(FNV1A( "mp_damage_scale_t_head" ) );
	static auto mp_damage_scale_ct_body = interfaces::m_cvar_system->find_var(FNV1A( "mp_damage_scale_ct_body" ) );
	static auto mp_damage_scale_t_body = interfaces::m_cvar_system->find_var(FNV1A( "mp_damage_scale_t_body" ) );

	auto team = e->get_team( );
	auto head_scale = team == 2 ? mp_damage_scale_ct_head->get_float( ) : mp_damage_scale_t_head->get_float( );
	auto body_scale = team == 2 ? mp_damage_scale_ct_body->get_float( ) : mp_damage_scale_t_body->get_float( );

	auto armor_heavy = e->has_heavy_armor( );
	auto armor_value = static_cast< float >( e->get_armor_value( ) );

	if ( armor_heavy ) head_scale *= 0.5f;

	switch ( hitgroup ){
	case HITGROUP_HEAD:
		current_damage = ( current_damage * 4.f ) * head_scale;
		break;
	case HITGROUP_CHEST:
	case 8:
		current_damage *= body_scale;
		break;
	case HITGROUP_STOMACH:
		current_damage = ( current_damage * 1.25f ) * body_scale;
		break;
	case HITGROUP_LEFT_ARM:
	case HITGROUP_RIGHT_ARM:
		current_damage *= body_scale;
		break;
	case HITGROUP_LEFT_LEG:
	case HITGROUP_RIGHT_LEG:
		current_damage = ( current_damage * 0.75f ) * body_scale;
		break;
	default:
		break;
	}

	static auto IsArmored = [ ]( c_cs_player* player, int hitgroup ){
		auto has_helmet = player->has_helmet( );
		auto armor_value = static_cast< float >( player->get_armor_value( ) );

		if ( armor_value > 0.f ){
			switch ( hitgroup ){
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFT_ARM:
			case HITGROUP_RIGHT_ARM:
			case 8:
				return true;
				break;
			case HITGROUP_HEAD:
				return has_helmet || ( bool )player->has_heavy_armor( );
				break;
			default:
				return ( bool )player->has_heavy_armor( );
				break;
			}
		}

		return false;
	};

	if ( IsArmored( e, hitgroup ) ){
		auto armor_scale = 1.f;
		auto armor_ratio = ( weapon_info->m_armor_ratio * 0.5f );
		auto armor_bonus_ratio = 0.5f;

		if ( armor_heavy ){
			armor_ratio *= 0.2f;
			armor_bonus_ratio = 0.33f;
			armor_scale = 0.25f;
		}

		float new_damage = current_damage * armor_ratio;
		float estiminated_damage = ( current_damage - ( current_damage * armor_ratio ) ) * ( armor_scale * armor_bonus_ratio );
		if ( estiminated_damage > armor_value ) new_damage = ( current_damage - ( armor_value / armor_bonus_ratio ) );

		current_damage = new_damage;
	}
}

bool penetration::handle_bullet_penetration( c_cs_weapon_data* info, FireBulletData_t& data, bool extracheck, vec3_t point ){
	c_game_trace trace_exit;
	surfacedata_t* enter_surface_data = interfaces::m_surface_data->get_surface_data( data.m_enter_trace.m_surface.m_surface_props );
	int enter_material = enter_surface_data->m_game.m_material;

	float enter_surf_penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;
	float final_damage_modifier = 0.18f;
	float compined_penetration_modifier = 0.f;
	bool solid_surf = ( ( data.m_enter_trace.m_contents >> 3 ) & CONTENTS_SOLID );
	bool light_surf = ( ( data.m_enter_trace.m_surface.m_flags >> 7 ) & SURF_LIGHT );

	if(data.m_penetration_count <= 0|| ( !data.m_penetration_count && !light_surf && !solid_surf && enter_material != CHAR_TEX_GLASS && enter_material != CHAR_TEX_GRATE )|| info->m_penetration <= 0.f
	|| !trace_to_exit( &data.m_enter_trace, data.m_enter_trace.m_end_pos, data.m_direction, &trace_exit ) && !( interfaces::m_trace_system->get_point_contents( data.m_enter_trace.m_end_pos, MASK_SHOT_HULL | CONTENTS_HITBOX, NULL ) & ( MASK_SHOT_HULL | CONTENTS_HITBOX ) )){
		return false;
	}

	surfacedata_t* exit_surface_data = interfaces::m_surface_data->get_surface_data( trace_exit.m_surface.m_surface_props );
	int exit_material = exit_surface_data->m_game.m_material;
	float exit_surf_penetration_modifier = exit_surface_data->m_game.m_penetration_modifier;

	static auto dmg_reduction_bullets = interfaces::m_cvar_system->find_var(FNV1A("ff_damage_reduction_bullets" ))->get_float( );
	static auto dmg_bullet_penetration = interfaces::m_cvar_system->find_var(FNV1A("ff_damage_bullet_penetration" ))->get_float( );

	auto ent = 
		reinterpret_cast< c_cs_player* >( data.m_enter_trace.m_hit_entity );

	if ( enter_material == CHAR_TEX_GRATE || enter_material == CHAR_TEX_GLASS ){
		compined_penetration_modifier = 3.0f;
		final_damage_modifier = 0.05f;
	}
	else if ( light_surf || solid_surf ){
		compined_penetration_modifier = 1.0f;
		final_damage_modifier = 0.16f;
	}
	else if ( enter_material == CHAR_TEX_FLESH && ent->get_team( ) != globals::m_local->get_team( ) && !dmg_reduction_bullets ){
		if ( !dmg_bullet_penetration )
			return false;

		compined_penetration_modifier = dmg_bullet_penetration;
		final_damage_modifier = 0.16f;
	}
	else{
		compined_penetration_modifier = ( enter_surf_penetration_modifier + exit_surf_penetration_modifier ) * 0.5f;
		final_damage_modifier = 0.16f;
	}

	if ( enter_material == exit_material ){
		if ( exit_material == CHAR_TEX_CARDBOARD || exit_material == CHAR_TEX_WOOD )
			compined_penetration_modifier = 3.f;
		else if ( exit_material == CHAR_TEX_PLASTIC )
			compined_penetration_modifier = 2.0f;
	}

	float thickness = ( trace_exit.m_end_pos - data.m_enter_trace.m_end_pos ).length_sqr( );
	float modifier = std::max( 0.f, 1.f / compined_penetration_modifier );

	if ( extracheck ) {
		static auto VectortoVectorVisible = [ & ]( vec3_t src, vec3_t point ) -> bool {
			c_game_trace TraceInit;
			trace_line( src, point, MASK_SOLID, globals::m_local, &TraceInit );
			c_game_trace Trace;
			trace_line( src, point, MASK_SOLID, ( c_cs_player* )TraceInit.m_hit_entity, &Trace );

			if ( Trace.m_fraction == 1.0f || TraceInit.m_fraction == 1.0f )
				return true;

			return false;
		};

		if ( !VectortoVectorVisible( trace_exit.m_end_pos, point ) )
			return false;
	}

	float lost_damage = std::max( ( ( modifier * thickness ) / 24.f ) + ( ( data.m_current_damage * final_damage_modifier ) + ( std::max( 3.75f / info->m_penetration, 0.f ) * 3.f * modifier ) ), 0.f );

	if ( lost_damage > data.m_current_damage )
		return false;

	if ( lost_damage > 0.f )
		data.m_current_damage -= lost_damage;

	if ( data.m_current_damage < 1.f )
		return false;

	data.m_current_position = trace_exit.m_end_pos;
	data.m_penetration_count--;

	return true;
}

bool penetration::trace_to_exit( c_game_trace* enter_trace, vec3_t start, vec3_t dir, c_game_trace* exit_trace ){
	vec3_t end;
	float distance = 0.f;
	signed int distance_check = 25;
	int first_contents = 0;

	do{
		distance += 3.5f;
		end = start + dir * distance;

		if ( !first_contents ) first_contents = interfaces::m_trace_system->get_point_contents( end, MASK_SHOT | CONTENTS_GRATE, NULL );

		int point_contents = interfaces::m_trace_system->get_point_contents( end, MASK_SHOT | CONTENTS_GRATE, NULL );

		if ( !( point_contents & ( MASK_SHOT_HULL | CONTENTS_HITBOX ) ) || point_contents & CONTENTS_HITBOX && point_contents != first_contents ){
			vec3_t new_end = end - ( dir * 4.f );

			interfaces::m_trace_system->trace_ray(ray_t(end, new_end), MASK_SHOT | CONTENTS_GRATE, nullptr, exit_trace );

			if ( exit_trace->m_start_solid && exit_trace->m_surface.m_flags & SURF_HITBOX ){
				trace_line( end, start, MASK_SHOT_HULL | CONTENTS_HITBOX, ( c_cs_player* )exit_trace->m_hit_entity, exit_trace );

				if ( exit_trace->did_hit( ) && !exit_trace->m_start_solid ) return true;

				continue;
			}

			if ( exit_trace->did_hit( ) && !exit_trace->m_start_solid ){
				if ( enter_trace->m_surface.m_flags & SURF_NODRAW || !( exit_trace->m_surface.m_flags & SURF_NODRAW ) ) {
					if ( exit_trace->m_plane.m_normal.dot_product( dir ) <= 1.f )
						return true;

					continue;
				}

				if ( is_breakable_entity( ( c_cs_player* )enter_trace->m_hit_entity )
					&& is_breakable_entity( ( c_cs_player* )exit_trace->m_hit_entity ) )
					return true;

				continue;
			}

			if ( exit_trace->m_surface.m_flags & SURF_NODRAW ){
				if ( is_breakable_entity( ( c_cs_player* )enter_trace->m_hit_entity )
					&& is_breakable_entity( ( c_cs_player* )exit_trace->m_hit_entity ) ){
					return true;
				}
				else if ( !( enter_trace->m_surface.m_flags & SURF_NODRAW ) ){
					continue;
				}
			}

			if ( ( !enter_trace->m_hit_entity || enter_trace->m_hit_entity->get_index( ) == 0 ) && ( is_breakable_entity( ( c_cs_player* )enter_trace->m_hit_entity ) ) ){
				exit_trace = enter_trace;
				exit_trace->m_end_pos = start + dir;
				return true;
			}

			continue;
		}

		distance_check--;
	} while ( distance_check );

	return false;
}

void penetration::trace_line( vec3_t& start, vec3_t& end, unsigned int mask, c_cs_player* ignore, c_game_trace* trace ){
	c_trace_filter filter;
	filter.m_skip = ignore;

	interfaces::m_trace_system->trace_ray(ray_t(start, end) , mask, &filter, trace );
}

void penetration::clip_trace( vec3_t& start, vec3_t& end, c_cs_player* e, unsigned int mask, i_trace_filter* filter, c_game_trace* old_trace ){
	if ( !e )
		return;

	vec3_t mins = e->get_collideable( )->obb_mins( ), maxs = e->get_collideable( )->obb_maxs( );

	vec3_t dir( end - start );
	dir.normalized( );

	vec3_t center = ( maxs + mins ) / 2, pos( center + e->get_origin( ) );

	vec3_t to = pos - start;
	float range_along = dir.dot_product( to );

	float range;

	if ( range_along < 0.f ){
		range = -to.length( );
	}
	else if ( range_along > dir.length( ) ){
		range = -( pos - end ).length( );
	}
	else{
		auto ray( pos - ( dir * range_along + start ) );
		range = ray.length( );
	}

	if ( range <= 60.f ){
		c_game_trace trace;

		interfaces::m_trace_system->clip_ray_to_entity(ray_t(start, end), mask, e, &trace );

		if ( old_trace->m_fraction > trace.m_fraction ) *old_trace = trace;
	}
}

bool penetration::is_breakable_entity( c_cs_player* e ){
	if ( !e || !e->get_index( ) )
		return false;

	static auto is_breakable_fn = reinterpret_cast< bool( __thiscall* )( c_cs_player* ) >(
		SIG( "client.dll", "55 8B EC 51 56 8B F1 85 F6 74 68" ).get( ) );

	const auto result = is_breakable_fn( e );
	auto class_id = e->get_client_class( )->m_class_id;
	if ( !result &&
		( class_id == 10 ||
		class_id == 31 ||
		( class_id == 11 && e->get_collideable( )->get_solid( ) == 1 ) ) )
		return true;

	return result;
}

bool penetration::can_hit_floating_point( const vec3_t& point, const vec3_t& source ){
	static auto VectortoVectorVisible = [ & ]( vec3_t src, vec3_t point ) -> bool {
		c_game_trace TraceInit;
		trace_line( src, point, MASK_SOLID, globals::m_local, &TraceInit );
		c_game_trace Trace;
		trace_line( src, point, MASK_SOLID, ( c_cs_player* )TraceInit.m_hit_entity, &Trace );

		if ( Trace.m_fraction == 1.0f || TraceInit.m_fraction == 1.0f )
			return true;

		return false;
	};

	FireBulletData_t data;
	data.m_start = source;
	data.m_filter = new c_trace_filter( );
	data.m_filter->m_skip = globals::m_local;
	qangle_t angles = math::calc_angle( data.m_start, point ); ///
	math::angle_vectors( angles, data.m_direction ); ///
	data.m_direction.normalize( );

	data.m_penetration_count = 1;
	auto weaponData = globals::m_local->get_active_weapon( )->get_cs_weapon_data( );

	if ( !weaponData )
		return false;

	data.m_current_damage = ( float )weaponData->m_damage;
	vec3_t end = data.m_start + ( data.m_direction * weaponData->m_range );
	trace_line( data.m_start, end, MASK_SHOT | CONTENTS_HITBOX, globals::m_local, &data.m_enter_trace );

	if ( VectortoVectorVisible( data.m_start, point ) || handle_bullet_penetration( weaponData, data, true, point ) )
		return true;

	delete data.m_filter;
	return false;
}