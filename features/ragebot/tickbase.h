#pragma once
#include "../../globals.h"

class tick_base_system : public c_singleton< tick_base_system > {
public:
	struct ShiftData {
		int m_next_shift_amount;
		int m_ticks_to_shift;
		int m_current_shift;
		int m_stored_tickbase;
		int m_ticks_shifted_last;

		bool m_did_shift_before;
		bool m_should_attempt_shift;
		int	 m_needs_recharge;
		bool m_prepare_recharge;
		bool m_should_be_ready;
		bool m_can_shift_tickbase;
		bool m_should_disable;
	} m_shift_data;

	struct Prediction {
		int m_shifted_command;
		int m_shifted_ticks;
		int m_original_tickbase;
	} m_prediction;

	
	bool CanFireWeapon(float curtime);
	bool IsFiring(float curtime);

	void WriteUserCmd( c_bf_write* buf, c_user_cmd* incmd, c_user_cmd* outcmd );
	void PreMovement( );
	void PostMovement( );

};

#define tickbase tick_base_system::instance( )