#pragma once
#include "../globals.h"
#include "../features/features.h"

namespace hooks {
	void init();

	void undo();

	namespace d3d_device {
		namespace reset {
			constexpr auto index = 16u;
			using T = long(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
			long __stdcall fn(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_params);
		}

		namespace present {
			constexpr auto index = 17u;
			using T = long(__stdcall*)(IDirect3DDevice9*, RECT*, RECT*, HWND, RGNDATA*);
			long __stdcall fn(IDirect3DDevice9* device, RECT* src_rect, RECT* dest_rect, HWND dest_wnd_override, RGNDATA* dirty_region);
		}
	}

	namespace client_dll {
		namespace write_user_cmd {
			constexpr auto index = 24;
			using T = bool(__stdcall*)(int, void*, int, int, bool);
			bool __stdcall fn(int m_nSlot, void* m_pBuffer, int m_nFrom, int m_nTo, bool m_bNewCmd);
		}

		namespace frame_stage_notify {
			constexpr auto index = 37u;
			using T = void(__stdcall*)(e_client_frame_stage);
			void __stdcall fn(e_client_frame_stage stage);
		}
	}

	namespace engine {				
		namespace cl_move {
			constexpr auto index = 59u;
			void __cdecl fn(float m1, bool m2);
			using T = void(__cdecl*)(float, bool);
		}
		namespace fire_game_event {
			constexpr auto index = 59u;
			void __fastcall fn(void* ecx, void* edx);
			using T = void(__fastcall*)(void*, void*);
		}

		namespace get_viewangles {
			constexpr auto index = 19u;
			void __fastcall fn(void* ecx, void* edx, vec3_t& ang);
			using T = void(__fastcall*)(void*, void*, vec3_t&);
		}

		namespace is_connected {
			constexpr auto index = 27u;
			bool __fastcall fn(void* ecx, void* edx);
			using T = bool(__fastcall*)(void*, void*);
		}

		namespace is_hltv {
			constexpr auto index = 93u;
			bool __fastcall fn(void* ecx, void* edx);
			using T = bool(__fastcall*)(void*, void*);
		}

		namespace is_in_game {
			constexpr auto index = 26u;
			bool __fastcall fn(void* ecx, void* edx);
			using T = bool(__fastcall*)(void*, void*);
		}

		namespace is_paused {
			constexpr auto index = 90u;
			bool __fastcall fn(void* ecx, void* edx);
			using T = bool(__fastcall*)(void*, void*);
		}
	}

	namespace net_channel {
		namespace process_packet {
			constexpr auto index = 39u;
			using T = void(__thiscall*)(void*, void*, bool);
			void __fastcall fn(void* ecx, void* edx, void* packet, bool header);
		}

		namespace send_net_msg {
			constexpr auto index = 40u;
			using T = bool(__thiscall*)(i_net_channel*, i_net_msg&, bool, bool);
			bool __fastcall fn(i_net_channel* pNetChan, void* edx, i_net_msg& msg, bool bForceReliable, bool bVoice);
		}
	}

	namespace client_mode {
		namespace create_move {
			constexpr auto index = 24u;
			using T = bool(__stdcall*)(int, c_user_cmd*);
			bool __stdcall fn(int sequence_number, c_user_cmd* cmd);
		}

		namespace override_view {
			constexpr auto index = 18u;
			using T = void(__stdcall*)(view_setup_t*);
			void __stdcall fn(view_setup_t* view);
		}
	}

	namespace model_render {
		namespace draw_model_execute {
			constexpr auto index = 21u;
			using T = void(__thiscall*)(i_model_render*, void*, const draw_model_state_t&, const model_render_info_t&, matrix3x4_t*);
			void __fastcall fn(i_model_render* ecx, void* edx, void* context, const draw_model_state_t& state, const model_render_info_t& info, matrix3x4_t* bones);
		}
	}

	namespace predictions {
		namespace run_cmd {
			constexpr auto index = 19;
			using T = void(__thiscall*)(void*, c_cs_player*, c_user_cmd*, i_move_helper*);
			void __fastcall fn(void* ecx, void*, c_cs_player* player, c_user_cmd* ucmd, i_move_helper* helper);
		}

		namespace in_prediction {
			constexpr auto index = 14;
			using T = bool(__thiscall*)(void*);
			bool __fastcall fn(void* p);
		}
	}

	namespace panel {
		namespace paint_traverse {
			constexpr auto index = 41u;
			using T = void(__thiscall*)(void*, uint32_t, bool, bool);
			void __fastcall fn(void* ecx, void* edx, uint32_t id, bool force_repaint, bool allow_force);
		}
	}

	namespace surface {
		namespace lock_cursor {
			constexpr auto index = 67u;
			using T = void(__thiscall*)(i_surface*);
			void __fastcall fn(i_surface* ecx, void* edx);
		}
	}

	namespace player {
		namespace build_transformations {
			constexpr auto index = 189u;
			void __fastcall fn(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7);
			using T = void(__fastcall*)(void*, void*, int, int, int, int, int, int);
		}

		namespace calc_view {
			constexpr auto index = 276u;
			void __fastcall fn(void* ecx, void* edx, vec3_t& eye_origin, vec3_t& eye_angles, float& m_near, float& m_far, float& fov);
			using T = void(__fastcall*)(void*, void*, vec3_t&, vec3_t&, float&, float&, float&);
		}

		namespace do_extra_bones_processing {
			constexpr auto index = 197u;
			void __fastcall fn(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7);
			using T = void(__fastcall*)(void*, void*, int, int, int, int, int, int);
		}

		namespace eye_angles {
			constexpr auto index = 169u;
			using T = qangle_t * (__thiscall*)(c_cs_player*);
			qangle_t* __fastcall fn(c_cs_player* ecx, void* edx);
		}

		namespace standard_blending_rules {
			constexpr auto index = 205u;
			void __fastcall fn(void* ecx, void* edx, c_studio_hdr* hdr, vec3_t* pos, vec4_t* q, float curtime, int mask);
			using T = void(__fastcall*)(void*, void*, c_studio_hdr*, vec3_t*, vec4_t*, float, int);
		}

		namespace update_clientside_animations {
			constexpr auto index = 223u;
			void __fastcall fn(void* ecx, void* edx);
			using T = void(__fastcall*)(void*, void*);
		}

		namespace sv_cheats_get_bool {
			constexpr auto index = 13;
			using T = bool(__fastcall*)(PVOID);
			bool __fastcall fn(PVOID pConVar);
		}

		namespace is_box_visible {
			constexpr auto index = 32;
			using T = int32_t(__thiscall*)(i_engine_client*, vec3_t&, vec3_t&);
			int32_t __fastcall fn(i_engine_client* engine_client, uint32_t, vec3_t& min, vec3_t& max);
		}
	}

	namespace renderable {
		namespace setup_bones {
			constexpr auto index = 13u;
			using T = bool(__thiscall*)(i_client_renderable*, matrix3x4_t*, int, int, float);
			bool __fastcall fn(i_client_renderable* ecx, void* edx, matrix3x4_t* bones, int max_bones, int mask, float time);
		}
	}

	extern std::unique_ptr<memory::hook_t> m_d3d_device;
	extern std::unique_ptr<memory::hook_t> m_client_dll;
	extern std::unique_ptr<memory::hook_t> m_engine;
	extern std::unique_ptr<memory::hook_t> m_client_mode;
	extern std::unique_ptr<memory::hook_t> m_model_render;
	extern std::unique_ptr<memory::hook_t> m_net_channel;
	extern std::unique_ptr<memory::hook_t> m_prediction;
	extern std::unique_ptr<memory::hook_t> m_panel;
	extern std::unique_ptr<memory::hook_t> m_surface;
	extern std::unique_ptr<memory::hook_t> m_player;
	extern std::unique_ptr<memory::hook_t> m_cheats;
	extern std::unique_ptr<memory::hook_t> m_renderable;
}
