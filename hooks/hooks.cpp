#include "hooks.h"

namespace hooks {
	void init() {
		m_d3d_device = std::make_unique<memory::hook_t>(interfaces::m_d3d_device);
		m_d3d_device->hook(d3d_device::reset::index, d3d_device::reset::fn);
		m_d3d_device->hook(d3d_device::present::index, d3d_device::present::fn);

		m_client_dll = std::make_unique<memory::hook_t>(interfaces::m_client_dll);
		m_client_dll->hook(client_dll::frame_stage_notify::index, client_dll::frame_stage_notify::fn);
		m_client_dll->hook(client_dll::write_user_cmd::index, client_dll::write_user_cmd::fn);

		m_client_mode = std::make_unique<memory::hook_t>(interfaces::m_client_mode);
		m_client_mode->hook(client_mode::create_move::index, client_mode::create_move::fn);
		m_client_mode->hook(client_mode::override_view::index, client_mode::override_view::fn);

		m_model_render = std::make_unique<memory::hook_t>(interfaces::m_model_render);
		m_model_render->hook(model_render::draw_model_execute::index, model_render::draw_model_execute::fn);

		m_engine = std::make_unique<memory::hook_t>(interfaces::m_engine);
		m_engine->hook(engine::fire_game_event::index, engine::fire_game_event::fn);
		m_engine->hook(engine::get_viewangles::index, engine::get_viewangles::fn);
		m_engine->hook(engine::is_connected::index, engine::is_connected::fn);
		m_engine->hook(engine::is_hltv::index, engine::is_hltv::fn);
		m_engine->hook(engine::is_in_game::index, engine::is_in_game::fn);
		m_engine->hook(engine::is_paused::index, engine::is_paused::fn);
		m_engine->hook(hooks::player::is_box_visible::index, hooks::player::is_box_visible::fn);

		m_panel = std::make_unique<memory::hook_t>(interfaces::m_panel);
		m_panel->hook(panel::paint_traverse::index, panel::paint_traverse::fn);

		m_surface = std::make_unique<memory::hook_t>(interfaces::m_surface);
		m_surface->hook(surface::lock_cursor::index, surface::lock_cursor::fn);

		m_prediction = std::make_unique<memory::hook_t>(interfaces::m_prediction);
		m_prediction->hook(predictions::in_prediction::index, predictions::in_prediction::fn);
		m_prediction->hook(predictions::run_cmd::index, predictions::run_cmd::fn);

		m_player = std::make_unique<memory::hook_t>(c_cs_player::get_vtable());
		m_player->hook(player::build_transformations::index, player::build_transformations::fn);
		m_player->hook(player::calc_view::index, player::calc_view::fn);
		m_player->hook(player::do_extra_bones_processing::index, player::do_extra_bones_processing::fn);
		m_player->hook(player::eye_angles::index, player::eye_angles::fn);
		m_player->hook(player::standard_blending_rules::index, player::standard_blending_rules::fn);
		m_player->hook(player::update_clientside_animations::index, player::update_clientside_animations::fn);

		m_renderable = std::make_unique<memory::hook_t>(i_client_renderable::get_vtable());
		m_renderable->hook(renderable::setup_bones::index, renderable::setup_bones::fn);

		m_net_channel = std::make_unique<memory::hook_t>(interfaces::m_client_state->m_net_channel);
		m_net_channel->hook(net_channel::process_packet::index, net_channel::process_packet::fn);
		m_net_channel->hook(net_channel::send_net_msg::index, net_channel::send_net_msg::fn);

		static auto* sv_cheats_con = interfaces::m_cvar_system->find_var(FNV1A("sv_cheats"));
		m_cheats = std::make_unique<memory::hook_t>(sv_cheats_con);
		m_cheats->hook(hooks::player::sv_cheats_get_bool::index, hooks::player::sv_cheats_get_bool::fn);
	}

	void undo() {
		m_renderable->unhook();
		m_player->unhook();	
		m_surface->unhook();
		m_panel->unhook();
		m_engine->unhook();
		m_model_render->unhook();
		m_client_mode->unhook();
		m_client_dll->unhook();
		m_d3d_device->unhook();
		m_net_channel->unhook();
		m_prediction->unhook();
		m_cheats->unhook();
	}

	std::unique_ptr<memory::hook_t> m_d3d_device = nullptr;
	std::unique_ptr<memory::hook_t> m_engine = nullptr;
	std::unique_ptr<memory::hook_t> m_client_dll = nullptr;
	std::unique_ptr<memory::hook_t> m_client_mode = nullptr;
	std::unique_ptr<memory::hook_t> m_model_render = nullptr;
	std::unique_ptr<memory::hook_t> m_net_channel = nullptr;
	std::unique_ptr<memory::hook_t> m_prediction = nullptr;
	std::unique_ptr<memory::hook_t> m_panel = nullptr;
	std::unique_ptr<memory::hook_t> m_surface = nullptr;
	std::unique_ptr<memory::hook_t> m_player = nullptr;
	std::unique_ptr<memory::hook_t> m_cheats = nullptr;
	std::unique_ptr<memory::hook_t> m_renderable = nullptr;
}
