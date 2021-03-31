#include "../features.h"

Aimbot g_aimbot{ };;

void AimPlayer::OnNetUpdate( c_cs_player* player ) {
	bool reset = ( !m_cfg.ragebot.enabled || !player->is_alive() || !player->is_enemy() );

	if( reset ) {
		m_missed_shots = 0;
		m_last_resolve = 0;
		return;
	}

	// update player ptr.
	m_player = player;
}

void AimPlayer::OnRoundStart(c_cs_player* player ) {
	m_player = player;
	m_shots = 0;
	m_missed_shots = 0;
	m_last_resolve = 0;
	m_delta = 0;

	m_hitboxes.clear( );
}

void AimPlayer::SetupHitboxes( animation* record, bool history ) {
	if( !record )
		return;

	// reset hitboxes.
	m_hitboxes.clear( );

	if( globals::m_local->get_active_weapon()->get_item_definition_index() == C_WEAPON_ZEUSX27 ) {
		// hitboxes for the zeus.
		m_hitboxes.push_back( { HITBOX_STOMACH , HitscanMode::PREFER } );
		return;
	}

	// prefer, always.
	if( m_cfg.ragebot.baim_prefer_always)
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );

	// prefer, lethal.
	if(m_cfg.ragebot.baim_prefer_lethal)
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::LETHAL } );


	// prefer, in air.
	if(m_cfg.ragebot.baim_prefer_air && !( record->flags & FL_ONGROUND ) )
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );

	bool only{ false };

	// only, always.
	if(m_cfg.ragebot.baim_always || g_aimbot.m_force_body ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );
	}

	// only, health.
	if(m_cfg.ragebot.baim_always_health && record->player->get_health( ) <= ( int )m_cfg.ragebot.baim_always_health_amount) {
		only = true;
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );
	}

	// only, in air.
	if(m_cfg.ragebot.baim_always_air && !( record->flags & FL_ONGROUND ) ) {
		only = true;
		m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER } );
	}

	// only baim conditions have been met.
	// do not insert more hitboxes.
	if( only )
		return;

	if(m_cfg.ragebot.hitbox_head) {

		if(m_cfg.ragebot.prefer_safepoint) {
			m_hitboxes.push_back( { HITBOX_HEAD, HitscanMode::PREFER_SAFEPOINT, true } );
		}
		
		m_hitboxes.push_back( { HITBOX_HEAD, m_cfg.ragebot.prefered_hitbox == 0 ? HitscanMode::PREFER : HitscanMode::NORMAL, false } );
	}

	if(m_cfg.ragebot.hitbox_neck) {
		m_hitboxes.push_back( { HITBOX_NECK, HitscanMode::NORMAL, false } );
	}

	if(m_cfg.ragebot.hitbox_chest ) {
		if (m_cfg.ragebot.prefer_safepoint) {
			m_hitboxes.push_back({ HITBOX_LOWER_CHEST, HitscanMode::PREFER_SAFEPOINT, true });
		}
		
		m_hitboxes.push_back( { HITBOX_LOWER_CHEST, HitscanMode::NORMAL, true } );
		m_hitboxes.push_back( { HITBOX_CHEST, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_UPPER_CHEST, HitscanMode::NORMAL, false } );
	}
	if(m_cfg.ragebot.hitbox_stomach) {

		if(m_cfg.ragebot.prefer_safepoint) {
			m_hitboxes.push_back( { HITBOX_PELVIS, HitscanMode::PREFER_SAFEPOINT, true } );
			m_hitboxes.push_back( { HITBOX_STOMACH, HitscanMode::PREFER_SAFEPOINT, true } );
		}
	
		m_hitboxes.push_back( { HITBOX_PELVIS, m_cfg.ragebot.prefered_hitbox == 2 ? HitscanMode::PREFER : HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_STOMACH, m_cfg.ragebot.prefered_hitbox == 1 ? HitscanMode::PREFER : HitscanMode::NORMAL, false } );
	}

	if(m_cfg.ragebot.hitbox_arms) {
		m_hitboxes.push_back( { HITBOX_LEFT_UPPER_ARM, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_RIGHT_UPPER_ARM, HitscanMode::NORMAL, false } );
	}

	if(m_cfg.ragebot.hitbox_legs) {
		m_hitboxes.push_back( { HITBOX_LEFT_THIGH, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_RIGHT_THIGH, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_LEFT_CALF, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_RIGHT_CALF, HitscanMode::NORMAL, false } );
	}

	if(m_cfg.ragebot.hitbox_feet) {
		m_hitboxes.push_back( { HITBOX_LEFT_FOOT, HitscanMode::NORMAL, false } );
		m_hitboxes.push_back( { HITBOX_RIGHT_FOOT, HitscanMode::NORMAL, false } );
	}

}

void Aimbot::init( ) {
	// clear old targets.
	m_targets.clear( );

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = qangle_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;
	m_hitbox = -1;

	m_best_dist = std::numeric_limits< float >::max( );
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max( );
	m_best_height = std::numeric_limits< float >::max( );

	if (!tickbase->m_shift_data.m_did_shift_before && !tickbase->m_shift_data.m_should_be_ready)
		m_shoot_next_tick = false;

	m_current_sphere.clear();
}

void Aimbot::StripAttack( ) {
	if (globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_R8_REVOLVER)
		return;
	else
		globals::m_cmd->m_buttons.remove(~IN_ATTACK);
}

void Aimbot::think( ) {
	// do all startup routines.
	init( );

	// sanity.
	if( !globals::m_local->get_active_weapon() )
		return;

	// no grenades or bomb.
	if(globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_GRENADE || globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_C4)
		return;

	// we have no aimbot enabled.
	if( !m_cfg.ragebot.enabled)
		return;

	if( !globals::hvh::m_weapon_fire )
		StripAttack( );

	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_R8_REVOLVER && globals::m_revolver_cock != 0;

	// one tick before being able to shoot.
	if( revolver && globals::m_revolver_cock > 0 && globals::m_revolver_cock == globals::m_revolver_query ) {
		//globas::m_packet = false;
		return;
	}

	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	/*if (globals::hvh::m_weapon_fire && !globals::hvh::m_lag && !revolver) {
		//globals::m_packet = false;
		//StripAttack();
		return;
	}*/

	// no point in aimbotting if we cannot fire this tick.
	if( !globals::hvh::m_weapon_fire )
		return;

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= interfaces::m_global_vars->m_max_clients; ++i) {
		c_cs_player* player = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));

		if( !player || player == globals::	m_local )
			continue;

		if( !IsValidTarget( player ) )
			continue;

		AimPlayer* data = &m_players[ i - 1 ];
		if( !data )
			continue;

		m_targets.push_back(data);
	}

	// scan available targets... if we even have any.
	find( );

	// finally set data when shooting.
	apply( );
}

void Aimbot::find() {
	struct BestTarget_t { c_cs_player* player; AimPlayer* target; vec3_t pos; float damage; float min_damage; animation* record; int hitbox; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	float		 tmp_min_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.target = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;
	best.hitbox = -1;

	if (m_targets.empty())
		return;

	// iterate all targets.
	for (const auto& t : m_targets) {
		if (!t->m_player)
			continue;

		const auto ideal = g_animations->get_latest_animation(t->m_player);
		if (!ideal.has_value() || ideal.value()->dormant || (ideal.value()->player /*&& ideal.value( )->player->m_bGunGameImmunity( )*/))
			continue;

		t->SetupHitboxes(ideal.value(), false);
		if (t->m_hitboxes.empty())
			continue;

		// try to select best record as target.
		if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, ideal.value(), tmp_min_damage)) {
			if (SelectTarget(ideal.value(), tmp_pos, tmp_damage)) {
				// if we made it so far, set shit. 
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal.value();
				best.min_damage = tmp_min_damage;
				best.target = t;
			}
		}

		const auto last = g_animations->get_oldest_animation(t->m_player);
		if (!last.has_value() || last.value() == ideal.value() || last.value()->dormant || (last.value()->player /*&& last.value( )->player->m_bGunGameImmunity( )*/))
			continue;

		t->SetupHitboxes(last.value(), true);
		if (t->m_hitboxes.empty())
			continue;

		// rip something went wrong..
		if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, last.value(), tmp_min_damage)) {
			if (SelectTarget(last.value(), tmp_pos, tmp_damage)) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last.value();
				best.min_damage = tmp_min_damage;
				best.target = t;
			}
		}

		// we found a target we can shoot at and deal damage? fuck yeah. (THIS IS TEMPORARY TILL WE REPLACE THE TARGET SELECTION)
		if (best.damage > 0.f && best.player && best.record)
			break;
	}

	// verify our target and set needed data.
	if (best.player && best.record && best.target) {
		// calculate aim angle.
		math::vector_angles(best.pos - globals::m_local->get_eye_pos(), m_angle);

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;
		m_hitbox = best.hitbox;

		if (!m_target || m_target->is_dormant() || m_record->dormant || !m_damage || !(m_damage >= best.min_damage || (m_damage <= best.min_damage && m_damage >= m_target->get_health())))
			return;

		//g_inputpred.Predict();

		if (best.target != m_old_target) {
			m_shoot_next_tick = false;
		}

		bool on = m_cfg.ragebot.hitchance;
		bool hit = (!globals::m_local->get_anim_state()->m_on_ground && globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_SSG_08 && globals::m_local->get_active_weapon() && globals::m_local->get_active_weapon()->get_inaccuracy() < 0.009f) || (on && CheckHitchance(m_target, m_angle, m_record, best.hitbox));

		// set autostop shit.
		m_stop = !(globals::m_cmd->m_buttons.has(IN_JUMP)) && !hit;

		//	g_movement.AutoStop();

		// if we can scope.
		bool can_scope = globals::m_local->get_active_weapon() && globals::m_local->get_active_weapon()->get_zoom_level() == 0 && globals::m_local->get_active_weapon()->is_zoomable(true);

		if (can_scope) {
			// always.
			if (m_cfg.ragebot.auto_scope == 1) {
				globals::m_cmd->m_buttons.add(IN_ATTACK2);
				return;
			}

			// hitchance fail.
			else if (m_cfg.ragebot.auto_scope == 2 && on && !hit) {
				globals::m_cmd->m_buttons.add(IN_ATTACK2);
				return;
			}
		}

		if (hit || !on) {
			// these conditions are so cancer
			if (m_cfg.ragebot.autofire) {
					globals::m_cmd->m_buttons.add(IN_ATTACK);
			
				m_old_target = best.target;
			}
		}
	}
}

bool Aimbot::CheckHitchance(c_cs_player* player, const qangle_t& angle, animation* record, int hitbox ) {
	if (!globals::m_local->get_active_weapon()) return false;

	float HITCHANCE_MAX = 82.f;
	constexpr int   SEED_MAX = 255;

	vec3_t     start{ globals::m_local->get_eye_pos()}, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	float hitchance = m_cfg.ragebot.hitchance_amount;

	if (m_shoot_next_tick)
		HITCHANCE_MAX += 25;

	size_t     total_hits{ }, needed_hits{ ( size_t )std::ceil( (hitchance * SEED_MAX ) / HITCHANCE_MAX) };

	// get needed directional vectors.
	math::angle_vectors( angle, &fwd, &right, &up );

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = globals::m_local->get_active_weapon()->get_inaccuracy( );
	spread = globals::m_local->get_active_weapon()->get_spread( );

	//note - nico; LOL, I don't know why this was commented out.
	globals::m_local->get_active_weapon()->update_accuracy();

	// iterate all possible seeds.
	for( int i{ }; i <= SEED_MAX; ++i ) {
		// get spread.
		wep_spread = globals::m_local->get_active_weapon()->calculate_spread( i, inaccuracy, spread );

		// get spread direction.
		dir = ( fwd + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized( );

		// get end of trace.
		end = start + ( dir * globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_range );

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if( CanHit( start, end, record, hitbox ) )
			++total_hits;

		// we made it.
		if( total_hits >= needed_hits )
			return true;

		// we cant make it anymore.
		if( ( SEED_MAX - i + total_hits ) < needed_hits )
			return false;
	}

	return false;
}

bool AimPlayer::SetupHitboxPoints( animation* record, matrix3x4_t* bones, int index, std::vector< vec3_t >& points ) {
	// reset points.
	points.clear( );

	const model_t* model = record->player->get_model( );
	if( !model )
		return false;

	studiohdr_t* hdr = interfaces::m_model_info->get_studio_model( model );
	if( !hdr )
		return false;

	mstudiohitboxset_t* set = hdr->get_hitbox_set(record->player->get_hitbox_set( ) );
	if( !set )
		return false;

	mstudiobbox_t* bbox = set->get_hitbox( index );
	if( !bbox )
		return false;

	// get hitbox scales.
	float hscale = m_cfg.ragebot.multipoint_head_scale * 0.01f;

	// big inair fix.
	if( !( record->flags.has(FL_ONGROUND )))
		hscale = 0.7f;

	float bscale = m_cfg.ragebot.multipoint_stomach_scale * 0.01f;

	// these indexes represent boxes.
	if( bbox->m_radius <= 0.f ) {
		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		hooks::client_mode::create_move::m_angle_matrix( bbox->m_angle, rot_matrix );

		// apply the rotation to the entity input space (local).
		matrix3x4_t matrix;
		math::ConcatTransforms( bones[ bbox->m_bone ], rot_matrix, matrix );

		// extract origin from matrix.
		vec3_t origin = matrix.origin( );

		// compute raw center point.
		vec3_t center = ( bbox->m_obb_min + bbox->m_obb_max ) / 2.f;

		// the feet hiboxes have a side, heel and the toe.
		if( index == HITBOX_RIGHT_FOOT || index == HITBOX_LEFT_FOOT ) {
			float d1 = ( bbox->m_obb_min.z - center.z ) * 0.875f;

			// invert.
			if( index == HITBOX_LEFT_FOOT )
				d1 *= -1.f;

			// side is more optimal then center.
			points.push_back( { center.x, center.y, center.z + d1 } );

			if(m_cfg.ragebot.multipoint_feet) {
				// get point offset relative to center point
				// and factor in hitbox scale.
				float d2 = ( bbox->m_obb_min.x - center.x ) * hscale;
				float d3 = ( bbox->m_obb_max.x - center.x ) * hscale;

				// heel.
				points.push_back( { center.x + d2, center.y, center.z } );

				// toe.
				points.push_back( { center.x + d3, center.y, center.z } );
			}
		}

		// nothing to do here we are done.
		if( points.empty( ) )
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for( auto& p : points ) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot_product( matrix[ 0 ] ), p.dot_product( matrix[ 1 ] ), p.dot_product( matrix[ 2 ] ) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else {
		// factor in the pointscale.
		float r = bbox->m_radius * hscale;
		float br = bbox->m_radius * bscale;

		// compute raw center point.
		vec3_t center = ( bbox->m_obb_min + bbox->m_obb_max) / 2.f;

		// head has 5 points.
		if( index == HITBOX_HEAD ) {
			// add center.
			points.push_back( center );

			if(m_cfg.ragebot.multipoint_head) {
				// rotation matrix 45 degrees.
				// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
				// std::cos( deg_to_rad( 45.f ) )
				constexpr float rotation = 0.70710678f;

				// top/back 45 deg.
				// this is the best spot to shoot at.
				points.push_back( { bbox->m_obb_max.x + ( rotation * r ), bbox->m_obb_max.y + ( -rotation * r ), bbox->m_obb_max.z } );

				vec3_t right{ bbox->m_obb_max.x, bbox->m_obb_max.y, bbox->m_obb_max.z + r };

				// right.
				points.push_back(right);

				vec3_t left{ bbox->m_obb_max.x, bbox->m_obb_max.y, bbox->m_obb_max.z - r };

				// left.
				points.push_back(left);

				// back.
				points.push_back( { bbox->m_obb_max.x, bbox->m_obb_max.y - r, bbox->m_obb_max.z } );

				// get animstate ptr.
				c_anim_state* state = record->player->get_anim_state();

				// add this point only under really specific circumstances.
				// if we are standing still and have the lowest possible pitch pose.
				if( state && record->velocity.length( ) <= 0.1f && record->eye_angles.x <= state->m_min_body_pitch) {

					// bottom point.
					points.push_back( { bbox->m_obb_max.x - r, bbox->m_obb_max.y, bbox->m_obb_max.z } );
				}
			}
		}

		// body has 5 points.
		else if( index == HITBOX_STOMACH) {
			// center.
			points.push_back( center );

			// back.
			if(m_cfg.ragebot.multipoint_stomach)
				points.push_back( { center.x, bbox->m_obb_max.y - br, center.z } );
		}

		else if( index == HITBOX_PELVIS || index == HITBOX_UPPER_CHEST ) {
			// back.
			points.push_back( { center.x, bbox->m_obb_max.y - r, center.z } );
		}

		// other stomach/chest hitboxes have 2 points.
		else if( index == HITBOX_LOWER_CHEST || index == HITBOX_CHEST ) {
			// add center.
			points.push_back( center );

			// add extra point on back.
			if(m_cfg.ragebot.multipoint_chest)
				points.push_back( { center.x, bbox->m_obb_max.y - r, center.z } );
		}

		else if( index == HITBOX_RIGHT_CALF || index == HITBOX_LEFT_CALF ) {
			// add center.
			points.push_back( center );

			// half bottom.
			if( m_cfg.ragebot.multipoint_legs)
				points.push_back( { bbox->m_obb_max.x - ( bbox->m_radius / 2.f ), bbox->m_obb_max.y, bbox->m_obb_max.z } );
		}

		else if( index == HITBOX_RIGHT_THIGH || index == HITBOX_LEFT_THIGH ) {
			// add center.
			points.push_back( center );
		}

		// arms get only one point.
		else if( index == HITBOX_RIGHT_UPPER_ARM || index == HITBOX_LEFT_UPPER_ARM ) {
			// elbow.
			points.push_back( { bbox->m_obb_max.x + bbox->m_radius, center.y, center.z } );
		}

		// nothing left to do here.
		if( points.empty( ) )
			return false;

		// transform capsule points.
		for( auto& p : points )
			math::vector_transform( p, bones[ bbox->m_bone ], p );
	}

	return true;
}

bool AimPlayer::GetBestAimPosition( vec3_t& aim, float& damage, int& hitbox, animation* record, float& tmp_min_damage ) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	// get player hp.
	int hp = std::min( 100, record->player->get_health( ) );

	if(globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_ZEUS_X27) {
		dmg = pendmg = hp;
		pen = true;
	}

	else {
		dmg = m_cfg.ragebot.visible_minimal_damage;
		if(m_cfg.ragebot.visible_minimal_damage_hp)
			dmg = std::ceil( ( dmg / 100.f ) * hp );

		if( g_aimbot.m_override_damage ) {
			dmg = m_cfg.ragebot.minimal_damage_override;
		}

		pendmg = m_cfg.ragebot.penetration_minimal_damage;
		if(m_cfg.ragebot.penetration_minimal_damage_hp);
			pendmg = std::ceil( ( pendmg / 100.f ) * hp );

		if( g_aimbot.m_override_damage &&  m_cfg.ragebot.minimal_damage_override_pen ) {
			pendmg = m_cfg.ragebot.minimal_damage_override;
		}

		pen = m_cfg.ragebot.penetration;
	}

	// iterate hitboxes.
	for( const auto& it : m_hitboxes ) {
		done = false;

		// setup points on hitbox.
		if (!SetupHitboxPoints(record, record->bones, it.m_index, points)) {
			continue;
		}

		// iterate points on hitbox.
		for( const auto& point : points ) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = pen;
			in.m_target = record->player;
			in.m_from = globals::m_local;
			in.m_pos = point;

			// ignore mindmg.
			//if(/*(it.m_mode == HitscanMode::PREFER && it.m_safepoint) ||*/ it.m_mode == HitscanMode::LETHAL)
				//in.m_damage = in.m_damage_pen = 1.f;

			penetration::PenetrationOutput_t out;

			// tests for intersection with unresolved matrix, if it returns true, the point should (!) be a safe point
			bool is_safepoint = g_aimbot.CanHit(globals::m_local->get_eye_pos(), point, record, it.m_index, true, record->bones);

			// we only want safe pointable (nice word) hitboxes when forcing..
			if (!is_safepoint && g_aimbot.m_force_safepoint)
				continue;

			// we can hit p!
			if( penetration::run( &in, &out ) ) {
				tmp_min_damage = dmg; 

				// nope we did not hit head..
				if( it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD )
					continue;

				// prefered hitbox, just stop now.
				if (it.m_mode == HitscanMode::PREFER && !it.m_safepoint)
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if (it.m_mode == HitscanMode::LETHAL && out.m_damage >= record->player->get_health())
					done = true;

				// always prefer safe points if we want to.
				else if (it.m_mode == HitscanMode::PREFER_SAFEPOINT && it.m_safepoint && is_safepoint)
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if( it.m_mode == HitscanMode::NORMAL ) {
					// we did more damage.
					if( out.m_damage > scan.m_damage ) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point;

						scan.m_hitbox = it.m_index;

						scan.m_safepoint = it.m_safepoint;

						// if the first point is lethal
						// screw the other ones.
						if( point == points.front( ) && out.m_damage >= record->player->get_health( ) )
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if( done ) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_safepoint = it.m_mode == HitscanMode::PREFER && it.m_safepoint;
					scan.m_mode = it.m_mode;

					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if( done )
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if( scan.m_damage > 0.f ) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;

		return true;
	}

	return false;
}

bool Aimbot::SelectTarget( animation* record, const vec3_t& aim, float damage ) {
	float dist, fov, height;
	int   hp;

	switch( m_cfg.ragebot.target_selection ) {

		// distance.
	case 0:
		dist = globals::m_local->get_eye_pos().dist_to(record->origin);

		if( dist < m_best_dist ) {
			m_best_dist = dist;
			return true;
		}

		break;

		// damage.
	case 1:
		if( damage > m_best_damage ) {
			m_best_damage = damage;
			return true;
		}

		break;

		// lowest hp.
	case 2:
		// fix for retarded servers?
		hp = std::min( 100, record->player->get_health( ) );

		if( hp < m_best_hp ) {
			m_best_hp = hp;
			return true;
		}

		break;

		// least lag.
	case 3:
		if( record->lag < m_best_lag ) {
			m_best_lag = record->lag;
			return true;
		}

		break;

		// height.
	case 4:
		height = record->origin.z - globals::m_local->get_origin( ).z;

		if( height < m_best_height ) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

void Aimbot::apply( ) {
	bool attack, attack2;

	// attack states.
	attack = ( globals::m_cmd->m_buttons.has(IN_ATTACK) );
	attack2 = (globals::m_local->get_active_weapon()->get_item_definition_index() == WEAPON_R8_REVOLVER && globals::m_cmd->m_buttons.has(IN_ATTACK2));

	// ensure we're attacking.
	if( attack || attack2 ) {
		// choke every shot.
		globals::m_packet = false;

		if( m_shoot_next_tick )
			m_shoot_next_tick = false;

		if( m_target ) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if( m_record ) {
				globals::m_cmd->m_tick_count = TIME_TO_TICKS( m_record->sim_time + globals::hvh::m_lerp );
			}

			// set angles to target.
			globals::m_cmd->m_view_angles = m_angle;
		}

		// norecoil.
		static auto weapon_recoil_scale = interfaces::m_cvar_system->find_var(FNV1A("sv_maxunlag"));
		if( m_cfg.ragebot.norecoil)
			globals::m_cmd->m_view_angles -= globals::m_local->get_aim_punch_angle( ) * weapon_recoil_scale->get_float( );

		// store fired shot.
		if(tickbase->IsFiring(TICKS_TO_TIME(globals::m_local->get_tick_base()))) {
			g_shots.OnShotFire( m_target ? m_target : nullptr, m_target ? m_damage : -1.f, 1, m_target ? m_record : nullptr, m_hitbox );

			// set that we fired.
			globals::m_shot = true;
		}

		if (!m_shoot_next_tick && globals::hvh::m_goal_shift == 14 && tickbase->m_shift_data.m_should_attempt_shift && !(tickbase->m_shift_data.m_prepare_recharge || tickbase->m_shift_data.m_did_shift_before && !tickbase->m_shift_data.m_should_be_ready)) {
			m_shoot_next_tick = true;
		}
	}
}

bool Aimbot::CanHit(vec3_t start, vec3_t end, animation* record, int box, bool in_shot, matrix3x4_t* bones)
{
	//if (!record || !record->player)
	//	return false;

	//// backup player
	//const auto backup_origin = record->player->m_vecOrigin();
	//const auto backup_abs_origin = record->player->GetAbsOrigin();
	//const auto backup_abs_angles = record->player->GetAbsAngles();
	//const auto backup_obb_mins = record->player->m_vecMins();
	//const auto backup_obb_maxs = record->player->m_vecMaxs();
	//const auto backup_cache = record->player->m_BoneCache2();

	//// always try to use our aimbot matrix first.
	//auto matrix = m_current_matrix;

	//// this is basically for using a custom matrix.
	//if (in_shot)
	//	matrix = bones;

	//if (!matrix)
	//	return false;

	//const model_t* model = record->player->GetModel();
	//if (!model)
	//	return false;

	//studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	//if (!hdr)
	//	return false;

	//mstudiohitboxset_t* set = hdr->GetHitboxSet(record->player->m_nHitboxSet());
	//if (!set)
	//	return false;

	//mstudiobbox_t* bbox = set->GetHitbox(box);
	//if (!bbox)
	//	return false;

	//vec3_t min, max;
	//const auto IsCapsule = bbox->m_radius != -1.f;

	//if (IsCapsule) {
	//	math::VectorTransform(bbox->m_mins, matrix[bbox->m_bone], min);
	//	math::VectorTransform(bbox->m_maxs, matrix[bbox->m_bone], max);
	//	const auto dist = math::SegmentToSegment(start, end, min, max);

	//	if (dist < bbox->m_radius) {
	//		return true;
	//	}
	//}
	//else {
	//	CGameTrace tr;

	//	// setup trace data
	//	record->player->m_vecOrigin() = record->m_vecOrigin;
	//	record->player->SetAbsOrigin(record->m_vecOrigin);
	//	record->player->SetAbsAngles(record->m_angAbsAngles);
	//	record->player->m_vecMins() = record->m_vecMins;
	//	record->player->m_vecMaxs() = record->m_vecMaxs;
	//	record->player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(matrix);

	//	// setup ray and trace.
	//	g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, record->player, &tr);

	//	record->player->m_vecOrigin() = backup_origin;
	//	record->player->SetAbsOrigin(backup_abs_origin);
	//	record->player->SetAbsAngles(backup_abs_angles);
	//	record->player->m_vecMins() = backup_obb_mins;
	//	record->player->m_vecMaxs() = backup_obb_maxs;
	//	record->player->m_BoneCache2() = backup_cache;

	//	// check if we hit a valid player / hitgroup on the player and increment total hits.
	//	if (tr.m_entity == record->player && game::IsValidHitgroup(tr.m_hitgroup))
	//		return true;
	//}

	//return false;
}
