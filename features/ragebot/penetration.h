#pragma once
#include "../../globals.h"

struct ReturnInfo_t
{
	int				m_damage;
	int				m_hitgroup;
	int				m_penetration_count;
	bool			m_did_penetrate_wall;
	float			m_thickness;
	vec3_t			m_end;
	c_cs_player*	m_hit_entity;

	ReturnInfo_t( int damage, int hitgroup, int penetration_count, bool did_penetrate_wall, float thickness, c_cs_player* hit_entity )
	{
		m_damage = damage;
		m_hitgroup = hitgroup;
		m_penetration_count = penetration_count;
		m_did_penetrate_wall = did_penetrate_wall;
		m_thickness = thickness;
		m_hit_entity = hit_entity;
	}
};

class penetration : public c_singleton<penetration>
{
private:

	struct FireBulletData_t
	{
		vec3_t				 m_start;
		vec3_t				 m_end;
		vec3_t				 m_current_position;
		vec3_t				 m_direction;

		c_trace_filter*		 m_filter;
		c_game_trace		 m_enter_trace;

		float				 m_thickness;
		float				 m_current_damage;
		int					 m_penetration_count;
	};

	void scale_damage( c_cs_player* e, c_cs_weapon_data* weapon_info, int hitgroup, float& current_damage );
	bool handle_bullet_penetration( c_cs_weapon_data* info, FireBulletData_t& data, bool extracheck = false, vec3_t point = vec3_t( 0, 0, 0 ) );
	bool trace_to_exit( c_game_trace* enter_trace, vec3_t start, vec3_t dir, c_game_trace* exit_trace );
	void trace_line( vec3_t& start, vec3_t& end, unsigned int mask, c_cs_player* ignore, c_game_trace* trace );
	void clip_trace( vec3_t& start, vec3_t& end, c_cs_player* e, unsigned int mask, i_trace_filter* filter, c_game_trace* old_trace );
	bool is_breakable_entity( c_cs_player* e );

	float hitgroup_damage( int iHitGroup );

public:
	std::vector<float>	scanned_damage;
	std::vector<vec3_t> scanned_points;

	void reset( )
	{
		scanned_damage.clear( );
		scanned_points.clear( );
	}

	bool can_hit_floating_point( const vec3_t& point, const vec3_t& source );
	ReturnInfo_t think( vec3_t pos, c_cs_player* target, int specific_hitgroup = -1 );
};

#define g_penetrate penetration::instance( )