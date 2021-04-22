#pragma once
#include "ai_unit.hpp"
#include "neural_network.hpp"
#include "game.hpp"


const std::vector<uint64_t> architecture = { 2, 6, 4, 1 };


struct Agent : public AiUnit
{
	Agent()
		: AiUnit(architecture)
	{}

	Agent(const Player &player_)
		: AiUnit(architecture)
		, player(player_)
	{
	}

	void process(const std::vector<float>& outputs) override
	{
		if (outputs[0] > 0.5f) {
			player.jump();
		}
	}

	Player player;
};
