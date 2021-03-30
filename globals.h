#pragma once
#include "common_includes.h"
#include "sdk/interfaces.h"
#include "sdk/_classes.h"

#include "singleton.h"
#include "menu/menu.h"

class NetPos {
public:
	float  m_time;
	vec3_t m_pos;

public:
	__forceinline NetPos() : m_time{ }, m_pos{ } {};
	__forceinline NetPos(float time, vec3_t pos) : m_time{ time }, m_pos{ pos } {};
};

namespace globals {
	namespace angles {
		extern qangle_t  m_anim;
		extern qangle_t	 m_strafe_angles;
	}

	namespace backup {
		extern bool		m_override_velocity;
		extern float	m_fl_velocity;
		extern int		m_tick;
	}

	namespace hvh {
		extern int m_rate;
		extern int m_goal_shift;

		extern int m_missed_shots;

		extern bool m_weapon_fire;
		extern bool m_pressing_move;
		extern bool	m_negate_desync;

		extern int  m_lag;
		extern float m_lerp;

		extern bool m_should_lag;
		extern int	m_wanted_choke;
	}

	extern HMODULE					m_module;
	extern c_local_player			m_local;
	extern c_user_cmd* m_cmd;
	extern qangle_t				    m_rotation;
	extern std::deque< NetPos >		m_net_pos;

	extern bool					    m_animate;
	extern bool					    m_update_fake;

	extern bool						m_packet;
	extern bool						m_shot;
	extern bool						m_old_shot;

	extern bool						m_call_update;
	extern bool						m_call_client_update;
	extern bool						m_call_bone;
	extern bool						m_call_client_update_enemy;
	extern bool						m_force_bone;
	extern int		                m_shot_command_number;

	extern e_client_frame_stage		m_cur_stage;
}