#include "../hooks.h"
#include "../../menu/menu.h"

long __stdcall hooks::d3d_device::present::fn(IDirect3DDevice9* device, RECT* src_rect, RECT* dest_rect, HWND dest_wnd_override, RGNDATA* dirty_region) {
	static const auto original = m_d3d_device->get_original<T>(index);

	IDirect3DVertexDeclaration9* vert_dec;
	if (device->GetVertexDeclaration(&vert_dec))
		return original(device, src_rect, dest_rect, dest_wnd_override, dirty_region);

	IDirect3DVertexShader9* vert_shader;
	if (device->GetVertexShader(&vert_shader))
		return original(device, src_rect, dest_rect, dest_wnd_override, dirty_region);

	if (device) {
		tickbase->m_shift_data.m_should_attempt_shift = m_cfg.ragebot.main.enabled /*&& GetKeyState(m_cfg.ragebot.main.exploit_key)*/;

		bool old_tickbase = tickbase->m_shift_data.m_should_attempt_shift;
		globals::hvh::m_goal_shift = m_cfg.ragebot.main.exploit_type == 0 ? 16 : 6;

		if (old_tickbase != tickbase->m_shift_data.m_should_attempt_shift) {

			if (tickbase->m_shift_data.m_should_attempt_shift)
				tickbase->m_shift_data.m_needs_recharge = globals::hvh::m_goal_shift;
			else
				tickbase->m_shift_data.m_needs_recharge = 0;

			tickbase->m_shift_data.m_did_shift_before = false;
		}
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	menu->on_paint();

	render::add_to_draw_list();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	device->SetVertexShader(vert_shader);
	device->SetVertexDeclaration(vert_dec);

	return original(device, src_rect, dest_rect, dest_wnd_override, dirty_region);
}

long __stdcall hooks::d3d_device::reset::fn(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_params) {
	static const auto original = m_d3d_device->get_original<T>(index);

	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto ret = original(device, present_params);
	ImGui_ImplDX9_CreateDeviceObjects();

	return ret;
}