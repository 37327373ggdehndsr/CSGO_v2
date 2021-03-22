#include "../hooks.h"

void __fastcall hooks::player::build_transformations::fn(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7){
	static const auto original = m_player->get_original<T>(index);

	auto player = (c_cs_player*)ecx;
	if (!player)
		return original(ecx, edx, a2, a3, a4, a5, a6, a7);

	auto o_jiggle_bones_enabled = player->m_jiggle_bones();

	player->m_jiggle_bones() = false;

	original(ecx, edx, a2, a3, a4, a5, a6, a7);

	player->m_jiggle_bones() = o_jiggle_bones_enabled;
}

void __fastcall hooks::player::calc_view::fn(void* ecx, void* edx, vec3_t& eye_origin, vec3_t& eye_angles, float& m_near, float& m_far, float& fov) {
	static const auto original = m_player->get_original<T>(index);

	auto player = (c_cs_player*)ecx;
	if (!player || player->get_index() != interfaces::m_engine->get_local_player())
		return original(ecx, edx, eye_origin, eye_angles, m_near, m_far, fov);

	const auto o_new_animstate = player->get_new_anim_state();

	player->get_new_anim_state() = false;

	original(ecx, edx, eye_origin, eye_angles, m_near, m_far, fov);

	player->get_new_anim_state() = o_new_animstate;
}

void __fastcall hooks::player::do_extra_bones_processing::fn(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7){
	static const auto original = m_player->get_original<T>(index);

	auto player = (c_cs_player*)ecx;
	if (!player) {
		original(ecx, edx, a2, a3, a4, a5, a6, a7);
		return;
	}

	auto animstate = player->get_anim_state();
	if (!animstate || !animstate->m_base_entity) {
		original(ecx, edx, a2, a3, a4, a5, a6, a7);
		return;
	}

	const auto o_on_ground = animstate->m_on_ground;

	animstate->m_on_ground = false;

	original(ecx, edx, a2, a3, a4, a5, a6, a7);

	animstate->m_on_ground = o_on_ground;
}

qangle_t* __fastcall hooks::player::eye_angles::fn(c_cs_player* ecx, void* edx) {
	static const auto original = m_player->get_original<T>(index);

	if (ecx != globals::m_local)
		return original(ecx);

	static const auto return_to_anim_state_yaw = SIG("client.dll", "F3 0F 10 55 ? 51 8B 8E ? ? ? ?");
	static const auto return_to_anim_state_pitch = SIG("client.dll", "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55 ?");

	const auto ret = memory::stack_t().ret();
	if (ret == return_to_anim_state_yaw
		|| ret == return_to_anim_state_pitch)
		return &globals::angles::m_anim;

	return original(ecx);
}

void __fastcall hooks::player::standard_blending_rules::fn(void* ecx, void* edx, c_studio_hdr* hdr, vec3_t* pos, vec4_t* q, float curtime, int mask) {
	static const auto original = m_player->get_original<T>(index);

	auto player = (c_cs_player*)ecx;
	if (!player) {
		return original(ecx, 0, hdr, pos, q, curtime, mask);
	}

	*(uint32_t*)((uintptr_t)player + 0xf0) |= 8;

	original(ecx, edx, hdr, pos, q, curtime, mask);

	*(uint32_t*)((uintptr_t)player + 0xf0) &= ~8;
}

void __fastcall hooks::player::update_clientside_animations::fn(void* ecx, void* edx) {
	static const auto original = m_player->get_original<T>(index);
	
	auto player = (c_cs_player*)ecx;
	if (!player || !player->is_player() || player->is_dormant()) {
		original(ecx, edx);
		return;
	}

	original(ecx, edx);
}

int32_t __fastcall hooks::player::is_box_visible::fn(i_engine_client* engine_client, uint32_t, vec3_t& min, vec3_t& max)
{
	static auto BoxVisible = m_engine->get_original<T>(index);

	static const auto ret = _("\x85\xC0\x74\x2D\x83\x7D\x10\x00\x75\x1C");

	if (!memcmp(_ReturnAddress(), ret, 10))
		return 1;

	return BoxVisible(engine_client, min, max);
}

bool __fastcall hooks::player::sv_cheats_get_bool::fn(PVOID pConVar){
	static auto dwCAM_Think = SIG("client.dll", "85 C0 75 30 38 86").get();
	static auto ofunc = m_cheats->get_original<T>(index);;

	if (!ofunc)
		return false;

	if (reinterpret_cast<DWORD>(_ReturnAddress()) == reinterpret_cast<DWORD>(dwCAM_Think))
		return true;

	return ofunc(pConVar);
}
