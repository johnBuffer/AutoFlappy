#include <SFML/Graphics.hpp>
#include <vector>
#include <list>
#include <event_manager.hpp>
#include <iostream>

#include "game.hpp"
#include "dna.hpp"
#include "selector.hpp"
#include "number_generator.hpp"
#include "ai_player.hpp"
#include "neural_renderer.hpp"
#include "graph.hpp"


uint32_t aliveCounts(const std::vector<Agent>& players)
{
	uint32_t alive_counts = 0;
	for (const Agent& p : players) {
		alive_counts += !(p.player.collide);
	}

	return alive_counts;
}

int main()
{
	NumberGenerator<>::initialize();

	const uint32_t win_width = 1600;
	const uint32_t win_height = 900;
	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;
	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "AutoFlappy", sf::Style::Default, settings);
	//window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	bool slow_motion = false;
	const float base_dt = 0.016f;
	float dt = base_dt;

	sfev::EventManager event_manager(window);
	event_manager.addEventCallback(sf::Event::Closed, [&](sfev::CstEv ev) { window.close(); });
	event_manager.addKeyPressedCallback(sf::Keyboard::Escape, [&](sfev::CstEv ev) { window.close(); });

	const uint32_t pop_size = 200;
	Selector<Agent> stadium(pop_size);
	std::vector<sf::Color> colors({ sf::Color(80, 81, 79), 
		                            sf::Color(121, 85, 83),
									sf::Color(161, 88, 86),
									sf::Color(242, 95, 92),
									sf::Color(249, 160, 97),
									sf::Color(255, 224, 102),
									sf::Color(146, 174, 131),
									sf::Color(36, 123, 160),
									sf::Color(74, 158, 170),
									sf::Color(112, 193, 179) });

	bool full_speed = false;
	event_manager.addKeyPressedCallback(sf::Keyboard::E, [&](sfev::CstEv ev) { full_speed = !full_speed; window.setFramerateLimit((!full_speed)*60); });
	event_manager.addKeyPressedCallback(sf::Keyboard::Space, [&](sfev::CstEv ev) { slow_motion = !slow_motion; dt = slow_motion ? base_dt * 0.1f : base_dt; });

	sf::Font font;
	font.loadFromFile("res/font.ttf");

	const float text_margin = 10.0f;
	sf::Text score;
	score.setFillColor(sf::Color::White);
	score.setFont(font);
	score.setCharacterSize(20);

	sf::Text current_generation = score;
	current_generation.setCharacterSize(32);
	current_generation.setPosition(text_margin, 0.5f * text_margin);
	score.setPosition(text_margin, text_margin + 38);

	sf::Text generation_info = score;
	generation_info.setPosition(text_margin, text_margin + 62);
	sf::Text graph_caption = score;
	graph_caption.setString("Best fitness");

	sf::Text new_best = score;
	new_best.setString("New BEST!");
	new_best.setPosition(win_width - new_best.getGlobalBounds().width - text_margin, 24.0f + 1.5f * text_margin);
	sf::Text improvement = score;
	improvement.setCharacterSize(16);
	improvement.setPosition(win_width - 512 - text_margin, 24.0f + 1.5f * text_margin);
	improvement.setFillColor(sf::Color(96, 211, 148));

	World world(win_width, win_height);
	world.past_threshold = 120.0f;
	world.scroll_speed = 500.0f;
	world.initialize();
	Renderer renderer(world, window);

	sf::Texture crown;
	crown.loadFromFile("res/img.png");
	sf::RectangleShape crown_sprite;
	crown_sprite.setTexture(&crown);
	crown_sprite.setSize(sf::Vector2f(80.0, 80.0f));
	crown_sprite.setOrigin(sf::Vector2f(40.0, 70.0f));

	NeuralRenderer network_printer;
	const sf::Vector2f network_size = network_printer.getSize(4, 6);
	network_printer.position = sf::Vector2f(win_width - network_size.x - text_margin, win_height - network_size.y - text_margin);

	const float player_radius = 30.0f;

	const float graph_height = 100.0f;
	Graphic bestGraph(200, sf::Vector2f(600.0f, graph_height), sf::Vector2f(2.0f * text_margin, win_height - graph_height - 2.0f * text_margin));
	bestGraph.color = sf::Color(96, 211, 148);
	graph_caption.setPosition(2.0f * text_margin, win_height - graph_height - 3.0f * text_margin - 20.0f);
	const float back_height = bestGraph.height + 3.0f * text_margin + 20.0f;
	sf::RectangleShape graph_background(sf::Vector2f(bestGraph.width + 2.0f * text_margin, back_height));
	graph_background.setPosition(text_margin, win_height - back_height - text_margin);
	graph_background.setFillColor(sf::Color(100, 100, 100, 100));

	float best_score = 1.0f;
	while (window.isOpen()) {
		event_manager.processEvents();

		world.initialize();

		for (Agent& a : stadium.getCurrentPopulation()) {
			Player p;
			p.position = sf::Vector2f(150.0f, 250.0f);
			p.radius = player_radius;
			p.score = 0.0f;
			a.player = p;
		}

		float time = 0.0f;

		float avgFitness = 0.0f;
		uint32_t agents_remaining = 1;
		while (agents_remaining && time < 250.0f && window.isOpen()) {
			event_manager.processEvents();
			const sf::Vector2f next_hole_1 = world.getHole(0);
			const sf::Vector2f next_hole_2 = world.getHole(1);

			uint32_t agent_index = 0;
			for (Agent& agent : stadium.getCurrentPopulation()) {
				if (!agent.player.collide) {
					avgFitness += dt;
					const sf::Vector2f to_hole_1 = next_hole_1 - agent.player.position;
					const std::vector<float> inputs = { normalize(to_hole_1.x, win_width), normalize(to_hole_1.y, win_height) };
					agent.execute(inputs);
				}
			}

			time += dt;

			world.update(dt);
			for (Agent& agent : stadium.getCurrentPopulation()) {
				world.checkPlayer(agent.player, dt);
			}

			agents_remaining = aliveCounts(stadium.getCurrentPopulation());

			current_generation.setString("Generation " + toString(stadium.current_iteration));
			score.setString("Current time " + toString(time));
			generation_info.setString("Remaining: " + toString(agents_remaining));

			// Graphics
			window.clear(sf::Color(191, 219, 247));
			window.draw(current_generation);
			window.draw(score);
			window.draw(generation_info);

			renderer.draw(world);

			if (!full_speed) {
				for (Agent& agent : stadium.getCurrentPopulation()) {
					if (!agent.player.collide) {
						const sf::Vector2f to_hole_1 = next_hole_1 - agent.player.position;
						const std::vector<float> inputs = { normalize(to_hole_1.x, win_width), normalize(to_hole_1.y, win_height) };
						network_printer.render(window, agent.network, inputs);
						break;
					}
				}
			}

			bestGraph.setLastValue(time);

			agent_index = 0;
			for (const Agent& p : stadium.getCurrentPopulation()) {
				if (!(p.player.collide)) {
					renderer.draw(p.player, colors[agent_index % 10]);
					if (time > best_score) {
						crown_sprite.setPosition(p.player.position);
						window.draw(crown_sprite);
					}
				}
				++agent_index;
			}

			// Gauge
			const float gauge_length = 512.0f;
			sf::RectangleShape gauge(sf::Vector2f(gauge_length, 24.0f));
			const float progress_ratio = std::min(1.0f, time / std::max(1.0f, best_score));
			sf::RectangleShape progress(sf::Vector2f(progress_ratio * gauge_length, 24.0f));
			gauge.setPosition(win_width - gauge_length - text_margin, text_margin);
			progress.setPosition(win_width - gauge_length - text_margin, text_margin);
			progress.setFillColor(sf::Color(96, 211, 148));
			window.draw(gauge);
			window.draw(progress);
			if (progress_ratio == 1.0f) {
				improvement.setString("+" + toString(time - best_score));
				window.draw(new_best);
				window.draw(improvement);
			}

			window.draw(graph_background);
			window.draw(graph_caption);
			bestGraph.render(window);

			window.display();
		}

		for (Agent& agent : stadium.getCurrentPopulation()) {
			agent.fitness = std::pow(1.0f * agent.player.score, 1.0f);
			if (agent.fitness > best_score) {
				best_score = agent.fitness;
			}
		}

		avgFitness = 0.0f;
		bestGraph.next();
		stadium.nextGeneration();
	}

	return 0;
}