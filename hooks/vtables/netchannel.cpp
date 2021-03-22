#include "../hooks.h"

void __fastcall hooks::net_channel::process_packet::fn(void* ecx, void* edx, void* packet, bool header){
	auto ProcessPacket = m_net_channel->get_original<T>(index);
	if (!interfaces::m_client_state->m_net_channel)
		return ProcessPacket(ecx, packet, header);

	ProcessPacket(ecx, packet, header);

	for (event_info_t* it{ interfaces::m_client_state->m_events }; it != nullptr; it = it->m_next) {
		if (!it->m_class_id)
			continue;

		it->m_delay = 0.f;
	}

	interfaces::m_engine->fire_events();
}

bool __fastcall hooks::net_channel::send_net_msg::fn(i_net_channel* pNetChan, void* edx, i_net_msg& msg, bool bForceReliable, bool bVoice){
	auto SendNetMsg = m_net_channel->get_original<T>(index);

	if (msg.get_type() == 14)
		return false;

	if (msg.get_group() == 9)
		bVoice = true;

	return SendNetMsg(pNetChan, msg, bForceReliable, bVoice);
}