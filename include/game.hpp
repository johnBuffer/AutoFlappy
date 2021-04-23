#pragma once
#include <SFML/Graphics.hpp>
#include <list>
#include <random>

#include "utils.hpp"
#include "number_generator.hpp"

// 1pxl = 1mm (arbitrary)
const float scale = 1000.0f;
const float jump_force = 1.0f;

struct Player
{
	Player()
		: collide(false)
		, jumping(false)
		, cooldown(0.0f)
		, score(0.0f)
		, v(0.0f)
	{}

	void update(const float dt)
	{
		if (!collide) {
			score += dt;
		}
		// Gravity
		v += 4.0f * scale * dt;
		// Speed
		position.y += v * dt;

		cooldown -= dt;
	}

	void jump()
	{
		v = -jump_force * scale;
	}

	void hit()
	{
		collide = true;
	}

	sf::Vector2f position;
	float v;
	float radius;
	bool collide;
	bool jumping;
	float cooldown;
	float score;
};


struct Obstacle
{
	Obstacle() = default;
	Obstacle(const sf::Vector2f& position_, const sf::Vector2f& size_)
		: position(position_)
		, size(size_)
	{}

	void update(const float dt, const float speed)
	{
		position.x -= speed * dt;
	}

	bool checkCollision(const Player& player) const
	{
		// Closest obstacle point
		sf::Vector2f pt = player.position;
		// X of closest point
		if      (pt.x > position.x + size.x * 0.5f) pt.x = position.x + size.x * 0.5f;
		else if (pt.x < position.x - size.x * 0.5f) pt.x = position.x - size.x * 0.5f;
		// Y of closest point
		if      (pt.y > position.y + size.y * 0.5f) pt.y = position.y + size.y * 0.5f;
		else if (pt.y < position.y - size.y * 0.5f) pt.y = position.y - size.y * 0.5f;
		return (getLength(pt - player.position) < player.radius);
	}

	sf::Vector2f position;
	sf::Vector2f size;
};


//const float hole_size = 180.0f;
const float hole_height_interval = 0.15f;


struct World
{
	World(const float width_, const float height_)
		: width(width_)
		, height(height_)
		, first_block(0)
		, generator(false)
		, hole_size(180.0f)
		, obstacle_width(80.0f)
	{
		initialize();
	}

	void initialize()
	{
		//generator.reset();
		obstacles.clear();
		const uint32_t count = 3;
		for (uint32_t i(0); i < count; ++i) {
			const float hole_height = height * 0.5f + generator.get(height * hole_height_interval);
			const float obstacle_1_height = height - hole_height - hole_size * 0.5f;
			const float obstacle_2_height = hole_height - hole_size * 0.5f;
			const float spacing = width / float(count) + obstacle_width * 0.5f;
			obstacles.emplace_back(sf::Vector2f(width + spacing * i, height - obstacle_1_height * 0.5f), sf::Vector2f(obstacle_width, obstacle_1_height));
			obstacles.emplace_back(sf::Vector2f(width + spacing * i, obstacle_2_height * 0.5f), sf::Vector2f(obstacle_width, obstacle_2_height));
		}
		first_block = 0;
	}

	void checkPlayer(Player& player, const float dt)
	{
		player.update(dt);

		if (std::isnan(player.position.y)) {
			player.hit();
			return;
		}

		const float radius_tolerance = 1.0f;
		if (player.position.y + radius_tolerance * player.radius < 0.0f || 
			player.position.y - radius_tolerance * player.radius> height) {
			player.hit();
			return;
		}

		for (const Obstacle& obstacle : obstacles) {
			if (obstacle.checkCollision(player)) {
				player.hit();
			}
		}
	}

	void update(const float dt)
	{
		const uint64_t size = obstacles.size() / 2u;
		for (uint64_t i(0); i < size; ++i) {
			obstacles[2 * i].update(dt, scroll_speed);
			obstacles[2 * i + 1].update(dt, scroll_speed);
			if (obstacles[2 * i].position.x < -obstacles[2 * i].size.x) {
				//const float hole_height = 250.0f + generator.get(180.0f);
				const float hole_height = height * 0.5f + generator.get(height * hole_height_interval);

				const float obstacle_1_height = height - hole_height - hole_size * 0.5f;
				const float obstacle_2_height = hole_height - hole_size * 0.5f;
				obstacles[2 * i + 0].size.y = obstacle_1_height;
				obstacles[2 * i + 1].size.y = obstacle_2_height;
				obstacles[2 * i + 0].position = sf::Vector2f(width + obstacle_width * 0.5f, height - obstacle_1_height * 0.5f);
				obstacles[2 * i + 1].position = sf::Vector2f(width + obstacle_width * 0.5f, obstacle_2_height * 0.5f);
			}
		}

		Obstacle& first_obs = getObstacle(0);
		if (first_obs.position.x + first_obs.size.x * 0.5f < past_threshold) {
			first_block += 2;
			first_block = first_block % obstacles.size();
		}
	}

	Obstacle& getObstacle(const uint32_t index)
	{
		return obstacles[(first_block + 2*index) % obstacles.size()];
	}

	sf::Vector2f getHole(const uint32_t index)
	{
		return getHole(getObstacle(index));
	}

	sf::Vector2f getHole(const Obstacle& obs) const
	{
		return sf::Vector2f(obs.position.x + obs.size.x, obs.position.y - obs.size.y * 0.5f - hole_size * 0.5f);
	}

	std::vector<Obstacle> obstacles;
	float scroll_speed;
	float width;
	float height;
	float hole_size;
	float obstacle_width;
	uint32_t first_block;
	float past_threshold;
	NumberGenerator<> generator;
};


struct Renderer
{
	Renderer(const World& world_, sf::RenderTarget& target_)
		: world(world_)
		, target(target_)
	{}

	void draw(const Player& player, const sf::Color& color = sf::Color::White)
	{
		if (player.collide) {
			return;
		}

		sf::CircleShape circle(player.radius);
		circle.setOrigin(player.radius, player.radius);
		circle.setPosition(player.position);
		circle.setFillColor(color);
		target.draw(circle);
	}

	void draw(const Obstacle& obstacle)
	{
		sf::RectangleShape shape(obstacle.size);
		shape.setFillColor(sf::Color(79, 93, 117));
		shape.setOrigin(0.5f * obstacle.size);
		shape.setPosition(obstacle.position);
		target.draw(shape);
	}

	void draw(const World& world)
	{
		for (const Obstacle& obstacle : world.obstacles) {
			draw(obstacle);
		}

		sf::RectangleShape shape(sf::Vector2f(2000.0f, 2000.0f));
		shape.setFillColor(sf::Color::White);
		shape.setPosition(sf::Vector2f(0.0f, world.width));
		target.draw(shape);
	}

	const World& world;
	sf::RenderTarget& target;
};
