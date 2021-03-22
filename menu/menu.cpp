#include "menu.h"

void c_menu::on_paint() {
	if (!(input::m_blocked = input::get_key<TOGGLE>(VK_INSERT)))
		return;

	ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2.f, ImGuiCond_Once, ImVec2(0.5f, 0.5f));

	ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_Once);

	if (ImGui::Begin(_("wok sdk v2"), 0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) {
		
		ImGui::Text("AA");
		ImGui::Checkbox("Enabled", &m_cfg.hvh.enabled);
		const char* pitch[] = {
			"Off",
			"down",
			"up",
			"ideal"
		};
		ImGui::Combo("Pitch", &m_cfg.hvh.pitch, pitch, IM_ARRAYSIZE(pitch));
		const char* attarget[] = {
			"Off",
			"away crosshair",
			"away distance",
		};
		ImGui::Combo("AtTarget", &m_cfg.hvh.base_angle, attarget, IM_ARRAYSIZE(attarget));
		const char* yaw[] = {
			"off",
			"backwards",
			"random"
		};
		ImGui::Combo("Yaw", &m_cfg.hvh.yaw, yaw, IM_ARRAYSIZE(yaw));

		const char* jitter[] = {
			"off",
			"offset",
			"random"
		};
		ImGui::Combo("jitter", &m_cfg.hvh.jitter_type, jitter, IM_ARRAYSIZE(jitter));
			ImGui::SliderInt("offset", &m_cfg.hvh.jitter_range, 0, 180);
		
		ImGui::Checkbox("Fake", &m_cfg.hvh.fake);
		ImGui::SliderInt("Fake delta", &m_cfg.hvh.max_fake_delta, 0, 60);

		ImGui::Checkbox("real around fake jitter", &m_cfg.hvh.enabled_around_fake_jitter);
		if (ImGui::BeginCombo("around mode", "Select")) {
			ImGui::Checkbox("stand", &m_cfg.hvh.real_around_fake_standing);
			ImGui::Checkbox("move", &m_cfg.hvh.real_around_fake_moving);
			ImGui::Checkbox("air", &m_cfg.hvh.real_around_fake_air);
			ImGui::Checkbox("slowwalk", &m_cfg.hvh.real_around_fake_slow_motion);
			ImGui::EndCombo();
		}

		ImGui::Checkbox("FakeLag", &m_cfg.hvh.fake_lag_enabled);
		const char* mode[] = {
			"max",
			"break",
			"random",
			"break step"
		};
		ImGui::Combo("FakeLag mode",&m_cfg.hvh.fake_lag_mode, mode, IM_ARRAYSIZE(mode));
		if (ImGui::BeginCombo("Fakelag", "Select")) {
			ImGui::Checkbox("stand", &m_cfg.hvh.fake_lag_on_stand);
			ImGui::Checkbox("move", &m_cfg.hvh.fake_lag_on_moving);
			ImGui::Checkbox("air", &m_cfg.hvh.fake_lag_on_air);
			ImGui::EndCombo();
		}
		ImGui::SliderInt("FakeLag limit", &m_cfg.hvh.fake_lag_limit, 2, 14);
		ImGui::Checkbox("FakeLag on peek", &m_cfg.hvh.fake_lag_on_peek);

		ImGui::Text("Misc");
		ImGui::Checkbox("unlock_inventory", &m_cfg.misc.unlock_inventory);


	}
	ImGui::End();
}