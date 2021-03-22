#include "globals.h"

namespace globals {
	namespace angles {
		qangle_t  m_anim = { };
		qangle_t  m_strafe_angles;
	}
	namespace backup {
		bool	m_override_velocity;
		float	m_fl_velocity;
		int		m_tick;
	}

	namespace hvh {
		int m_rate;
		int m_goal_shift;

		int m_missed_shots;

		bool m_pressing_move;
		bool m_negate_desync;

		int  m_lag;
		bool m_should_lag;
		int	 m_wanted_choke;
	}

	HMODULE					 m_module = nullptr;
	c_local_player			 m_local = {};
	c_user_cmd* m_cmd = nullptr;
	qangle_t				 m_rotation;
	std::deque< NetPos >	 m_net_pos;

	bool					 m_animate;
	bool					 m_update_fake;

	bool					 m_packet;
	bool					 m_shot;
	bool					 m_old_shot;

	bool					 m_call_update;
	bool					 m_call_client_update;
	bool					 m_call_bone;
	bool					 m_call_client_update_enemy;
	bool					 m_force_bone;

	int						 m_shot_command_number;

	e_client_frame_stage	 m_cur_stage;
}