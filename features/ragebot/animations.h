#pragma once
#include "../../globals.h"

struct animation
{
	animation( ) = default;
	explicit animation( c_cs_player* player );
	explicit animation( c_cs_player* player, qangle_t last_reliable_angle );
	void restore( c_cs_player* player );
	void apply( c_cs_player* player );
	void build_server_bones( c_cs_player* player );
	float get_client_interp_amount();
	bool is_valid(float m_sim_time, bool m_valid, const float range = 0.2f/*, float max_unlag */);
	///bool is_valid_extended( );

	c_cs_player*			player{ };
	int32_t					index{ };

	bool valid{}, has_anim_state{ };
	alignas( 16 ) matrix3x4_t bones[ 128 ];

	bool					dormant{ };
	vec3_t					velocity;
	vec3_t					origin;
	matrix3x4_t*			bone_cache;
	vec3_t					abs_origin;
	vec3_t					obb_mins;
	vec3_t					obb_maxs;
	anim_layer_t			layers[ 13 ];
	float					poses[ 24 ];
	c_anim_state*			anim_state;
	float					anim_time{ };
	float					sim_time{ };
	float					interp_time{ };
	float					duck{ };
	float					lby{ };
	float					last_shot_time{ };
	qangle_t				last_reliable_angle{ };
	qangle_t				eye_angles;
	qangle_t				abs_ang;
	bit_flag_t<uint32_t>	flags{ };
	bit_flag_t<uint32_t>	eflags{ };
	bit_flag_t<uint32_t>	effects{ };
	float					m_flFeetCycle{ };
	float					m_flFeetYawRate{ };
	int						lag{ };
	bool					didshot;
	std::string				resolver;
};

class c_animations : public c_singleton<c_animations> {
public:
	enum Modes : size_t {
		LEFT = 1,
		RIGHT = -1,
		BRUTE = 2,
	};
private:
	struct animation_info {
		animation_info( c_cs_player* player, std::deque<animation> animations )
			: player( player ), frames( std::move( animations ) ), last_spawn_time( 0 )
		{
		}

		void m_update_player( c_cs_player* player );
		void update_animations( animation* to, animation* from );

		c_cs_player*			player{ };
		std::deque<animation>	frames;
		float					m_brute{ };

		// last time this player spawned
		float					last_spawn_time;
		float					goal_feet_yaw;
		qangle_t				last_reliable_angle;
	};

	std::unordered_map<unsigned long, animation_info> animation_infos;

public:
	animation					m_anim;
	anim_layer_t				m_ResolverLayers[ 3 ][ 15 ];
	c_anim_state*				m_nState;
	matrix3x4_t					fake_matrix[ 128 ];
	matrix3x4_t					real_matrix[ 128 ];

	void manage_local_animations( );
	void manage_enemy_animations(c_cs_player* pEntity, animation* record);
	void manage_fake_animations( );
	void update_players( );
	
	anim_layer_t				 m_real_layers[ 13 ];
	anim_layer_t				 m_fake_layers[ 13] ;
	anim_layer_t				 m_backup_layers[ 13 ];
	
	c_anim_state*				 m_fake_state;
	c_anim_state*				 m_fake_state_allocated;

	float						 m_real_poses[ 24 ];
	float						 m_fake_poses[ 24 ];
	float						 m_backup_poses[ 24 ];
	vec3_t origin[ 128 ];
public:

	animation_info* get_animation_info( c_cs_player* player );
	std::optional<animation*> get_latest_animation( c_cs_player* player );
	std::optional<animation*> get_oldest_animation( c_cs_player* player );
	
	std::optional<std::pair<animation*, animation*>>  get_valid_animations( c_cs_player* player, float range = 1.0f );
	std::vector<animation*> get_latest_firing_animation( c_cs_player* player, float range = 1.0f);
};
#define g_animations c_animations::instance( )