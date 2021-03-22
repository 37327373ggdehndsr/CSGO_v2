#pragma once
#include "../../globals.h"

class hvh : public c_singleton<hvh> {
public:
    float    m_lby_update_time;
    int      m_next_random_update;
    int      m_random_angle;
    int      m_random_lag;
    int      m_direction;
public:
    bool     m_jitter_update;
    bool     m_use_real_around_fake;
    bool     m_slow_walk;
    bool     m_fake_duck;
    bool     m_step_switch;
    bool     m_in_lby_update;
    bool     m_in_balance_update;
    bool     m_can_micro_move;
    bool     m_should_resync;
public:
    void  run();
    void  pitch();
    void  yaw();
    void  adjustyaw();
    void  at_target();
    void  sendpacket();
    void  predict_lby();
    void  slowwalk();
    void  fakeduck();
};
#define g_hvh hvh::instance()