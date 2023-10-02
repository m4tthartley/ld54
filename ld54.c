// TODO
// Change line collision algorithm

#include <platform.h>
#include <im.h>
#include <timer.h>
#include <file.h>

#define DEBUG_MODE FALSE

#define SCREEN_LEFT -1280.0f/64.0f
#define SCREEN_RIGHT 1280.0f/64.0f
#define SCREEN_BOTTOM -720.0f/64.0f
#define SCREEN_TOP 720.0f/64.0f
#define SCREEN_WIDTH (SCREEN_RIGHT - SCREEN_LEFT)
#define SCREEN_HEIGHT (SCREEN_TOP - SCREEN_BOTTOM)

#define OBSTACLE_VARIENCE 0.25f

#define WALL_STEP 1.0f

#define WHITE (vec4_t){1, 1, 1, 1}
#define RED (vec4_t){1, 0, 0, 1}
#define GREEN (vec4_t){0, 1, 0, 1}
#define BLUE (vec4_t){0, 0, 1, 1}

vec2_t UP = {0.0f, 1.0f};

vec2_t player_model[] = {
	{-0.3f, -0.3f},
	{ 0.0f,  0.5f},
	{ 0.3f, -0.3f},
	{ 0.1f, -0.15f},
	{-0.1f, -0.15f},
	{-0.3f, -0.3f},
};

vec2_t player_engine[] = {
	{ 0.1f, -0.3f},
	{-0.1f, -0.3f},
	{ 0.0f, -0.6f},
	{ 0.1f, -0.3f},
};

void draw_line_strip(vec2_t* lines, int count) {
	FOR (i, count-1) {
		gfx_line(lines[i], lines[i+1]);
	}
}

vec2_t rotate2(vec2_t v, float r) {
	return vec2(sinf(r)*v.y + cosf(r)*v.x, sinf(r)*v.x + cosf(r)*v.y);
}

int ani_frame(float ani, float count) {
	return modf(ani, NULL) / (1.0f / count);
}

vec4_t obstacle_colors[] = {
	RED,
	GREEN,
	BLUE,
	WHITE,
};
typedef enum {
	ASTEROID_RED,
	ASTEROID_GREEN,
	ASTEROID_BLUE,
	ASTEROID_WHITE,

	ASTEROID_MAX,
} asteroid_type_t;

typedef enum {
	PLAYER = 1,
	OBSTACLE = 2,
	BULLET = 3,
	DEBRIS = 4,
	BLAST = 5,
	PARTICLE = 6,
	DEBUG_LINE = 7,
} entity_type_t;

typedef struct {
	enum {
		COLLIDER_NONE,
		COLLIDER_CIRCLE,
		COLLIDER_POINT,
		COLLIDER_DEATH,
	} type;
	float radius;
} collider_t;

typedef struct {
	entity_type_t type;
	vec2_t pos;
	vec2_t vel;
	float mass;
	float rot;
	float ani;
	float spin;
	float timer;
	collider_t collider;
	b32 dead;

	union {
		// Player properties
		struct {
			b32 on_bottom;
		};

		// Obstacle properties
		struct {
			asteroid_type_t obstacle_type;
			int life;
			vec4_t color;
			vec2_t points[20];
			int pointsCount;
		};

		// Debris properties
		struct {
			vec2_t l1;
			vec2_t l2;
		};

		// Debug line
		struct {
			vec2_t l1;
			vec2_t l2;
			vec4_t color;
		} debug_line;
	};

} entity_t;

typedef struct {
	entity_t* e1;
	union {
		entity_t* e2;
		vec2_t* line;
	};
} collision_t;

typedef struct {
	core_window_t window;
	core_timer_t timer;
	float dt;

	dynarr_t entities;
	collision_t collisions[64];
	int collision_count_forever;

	float up;

	vec2_t left_wall[64];
	vec2_t right_wall[64];

	m_arena assets;
	gfx_texture_t font;

} game_t;

entity_t create_obstacle(vec2_t pos, float size, vec4_t colour) {
    entity_t obstacle = {0};
	obstacle.type = OBSTACLE;
	obstacle.obstacle_type = r_int_range(0, ASTEROID_MAX);
    obstacle.pos = pos;
	obstacle.vel = vec2(r_float_range(0, 0.01f), r_float_range(0, 0.01f));
	obstacle.collider.radius = r_float_range(2.0f, 1.0f);
	obstacle.color = colour;

	float radius = size / 2.0f;
	obstacle.collider.radius = radius;
	obstacle.collider.type = COLLIDER_CIRCLE;
	obstacle.life = 1;


	float maxRadius = obstacle.collider.radius + OBSTACLE_VARIENCE;
	float minRadius = obstacle.collider.radius - OBSTACLE_VARIENCE;
	int numPoints = r_int_range(10, 19);

	for (int i = 0; i < numPoints; i++) {
        float angle = 2.0f * PI * ((float)i / (float)numPoints);
        float adjustedRadius = radius + (r_float_range(-OBSTACLE_VARIENCE, OBSTACLE_VARIENCE)); // assuming r_float_range(min, max)
        obstacle.points[i].x = adjustedRadius * cosf(angle);
        obstacle.points[i].y = adjustedRadius * sinf(angle);
    }

	obstacle.points[numPoints] = obstacle.points[0];
	obstacle.pointsCount = numPoints + 1;	

    return obstacle;
}

void add_glance_particles(game_t* game, vec2_t pos, vec2_t vel, float power, vec4_t color) {
	FOR (i, 10) {
		entity_t particle = {0};
		particle.type = PARTICLE;
		particle.pos = pos;
		particle.vel = add2(vel, vec2(r_float_range(-power, power), r_float_range(-power, power)));
		particle.collider.radius = 0.0f;
		particle.mass = 0.1f;
		particle.timer = r_float_range(0.1f, 0.5f);
		particle.color = color;
		dynarr_push(&game->entities, &particle);
	}
}

void draw_glance_particle(entity_t particle) {
	gfx_color(particle.color);
	glPointSize(1.0f);
	gfx_point(particle.pos);
}

void add_bullet(game_t* game, vec2_t pos, float rot) {
	entity_t bullet = {0};
	bullet.type = BULLET;
	bullet.pos = pos;
	bullet.vel = mul2f(rotate2(UP, rot), 2.0f);
	bullet.collider.type = COLLIDER_CIRCLE;
	bullet.collider.radius = 0.0f;
	bullet.mass = 0.1f;
	dynarr_push(&game->entities, &bullet);
}

void add_debris(game_t* game, vec2_t vel, vec2_t l1, vec2_t l2) {
	entity_t debris = {0};
	debris.type = DEBRIS;
	debris.pos = lerp2(l1, l2, 0.5f);
	debris.l1 = sub2(l1, debris.pos);
	debris.l2 = sub2(l2, debris.pos);
	float drift = 0.05f;
	debris.vel = add2(vec2(0,0), vec2(r_float_range(-drift, drift), r_float_range(-drift, drift)));
	debris.spin = r_float_range(0.2f, 1.0f);
	debris.timer = r_float_range(10.0f, 30.0f);
	dynarr_push(&game->entities, &debris);
}

void add_player(game_t* game) {
	entity_t player = {0};
	player.type = PLAYER;
	player.pos = vec2(5, 0);
	player.collider.type = COLLIDER_CIRCLE;
	player.collider.radius = 0.4f;
	dynarr_push(&game->entities, &player);
}

void add_blast(game_t* game, vec2_t pos, float size) {
	entity_t blast = {0};
	blast.type = BLAST;
	blast.pos = pos;
	blast.collider.type = COLLIDER_DEATH;
	blast.collider.radius = size;
	dynarr_push(&game->entities, &blast);
}

void player_die(game_t* game, entity_t* player) {
	FOR (i, array_size(player_model)) {
		add_debris(game, player->vel, add2(player->pos, rotate2(player_model[i], player->rot)), add2(player->pos, rotate2(player_model[i+1], player->rot)));
	}
	player->dead = TRUE;
}

void add_debug_line(game_t* game, vec2_t l1, vec2_t l2, vec4_t color) {
	entity_t line = {0};
	line.type = DEBUG_LINE;
	line.debug_line.l1 = l1;
	line.debug_line.l2 = l2;
	line.debug_line.color = color;
	dynarr_push(&game->entities, &line);
}

#include "collision.c"

void populate_obstacles(game_t* game) {
	entity_t obstacle;
	int added = 0;
	FOR (i, 400) {
		float size = 0;
		if (r_float() < 0.05f) {
			size = r_float_range(5.0f, 10.0f);
		} else {
			size = r_float_range(1.0f, 3.0f);
		}

		FOR (ii, 100) {
			obstacle = create_obstacle(
				vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT),
					r_float_range(SCREEN_TOP, (SCREEN_TOP*2)*3)),
				size,
				vec4(1, 1, 1, 1));
			b32 collision = FALSE;
			FORDYNARR (other, game->entities) {
				entity_t* other_obstacle = dynarr_get(&game->entities, other);
				if (entity_collision_test(&obstacle, other_obstacle)) {
					collision = TRUE;
					break;
				}
			}
			if (!collision) {
				dynarr_push(&game->entities, &obstacle);
				++added;
				break;
			}
		}
	}
	printf("added %i obstacles \n", added);
}

void initialise(game_t* game) {
	game->entities = dynarr(sizeof(entity_t));

	add_player(game);
	
    populate_obstacles(game);

	FOR (i, 64) {
		game->left_wall[i] = vec2(
				r_float_range(
					SCREEN_LEFT + 3, SCREEN_LEFT + 4),
					SCREEN_BOTTOM + (f32)i * WALL_STEP);
		game->right_wall[i] = vec2(
				r_float_range(
					SCREEN_RIGHT - 3, SCREEN_RIGHT - 4),
					SCREEN_BOTTOM + (f32)i * WALL_STEP);
	}

	m_stack(&game->assets, 0 ,0);
	m_reserve(&game->assets, GB(1), PAGE_SIZE);
	bitmap_t* fontbmp = f_load_font_file(&game->assets, "font.bmp");
	game->font = gfx_create_texture(fontbmp);
}

void restart(game_t* game) {
	core_window_t w = game->window;
	core_timer_t t = game->timer;
	VirtualFree(game->entities.arena.address,
		game->entities.arena.size, MEM_RELEASE);
	VirtualFree(game->assets.address,
		game->assets.size, MEM_RELEASE);

	m_zero(game, sizeof(game_t));
	game->window = w;
	game->timer = t;

	initialise(game);
}

void* start() {
	game_t* game = malloc(sizeof(game_t));
	m_zero(game, sizeof(game_t));
	core_window(&game->window, "LD54", 1280, 720, 0);
	core_opengl(&game->window);
	core_timer(&game->timer);

	initialise(game);
	
	return game;
}

void update_obstacle(game_t* game, entity_t* obstacle) {
	obstacle->mass = 5;
}

void draw_obstacle(entity_t* obstacle) {
	gfx_color(obstacle_colors[obstacle->obstacle_type]);
	if (DEBUG_MODE) {
		gfx_line_circle(obstacle->pos, obstacle->collider.radius * 2.0f, 30);
	}

	glPushMatrix();
	glTranslatef(obstacle->pos.x, obstacle->pos.y, 0.0f);
	draw_line_strip(obstacle->points, obstacle->pointsCount);
	glPopMatrix();

	if (DEBUG_MODE) {
		gfx_line(obstacle->pos, add2(obstacle->pos, mul2f(obstacle->vel, 10.0f)));
	}
}

void update_player(game_t* game, entity_t* player) {
	player->mass = 1;
	player->ani += game->dt * 10.0f;

	core_button_t* keyboard = game->window.keyboard;
	if (keyboard[KEY_UP].down) {
		player->vel = add2(player->vel, mul2f(rotate2(UP, player->rot), game->dt*1.0f));
	} else {
		player->ani = 0;
	}
	if (keyboard[KEY_LEFT].down) {
		player->rot -= game->dt * 3.0f;
	}
	if (keyboard[KEY_RIGHT].down) {
		player->rot += game->dt * 3.0f;
	}

	if (keyboard[KEY_SPACE].pressed) {
		add_bullet(game, add2(player->pos, rotate2(player_model[1], player->rot)), player->rot);
	}

	player->vel = mul2f(player->vel, 1.0f - 0.5f*game->dt);
}

void draw_player(entity_t* player) {
	gfx_color(WHITE);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(player->pos.x, player->pos.y, 0);
	glRotatef(todeg(player->rot), 0, 0, -1);
	draw_line_strip(player_model, array_size(player_model));
	if (ani_frame(player->ani, 1.4)) {
		draw_line_strip(player_engine, array_size(player_engine));
	}

	if (DEBUG_MODE) {
		gfx_color(vec4(1, 0, 1, 1));
		gfx_line_circle(vec2(0, 0), player->collider.radius*2, 10);
	}
	glPopMatrix();
}

void frame(game_t* game) {
	core_window_update(&game->window);
	core_timer_update(&game->timer);
	game->dt = game->timer.dt;
    
	gfx_clear(vec4(0, 0, 0.0f, 1));

	gfx_coord_system(1280.0f/64.0f, 720.0f/64.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1280.0f/64.0f, 1280.0f/64.0f,
			-720.0f/64.0f, 720.0f/64.0f,
			-10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPointSize(3.0f);

	game->up += game->dt * 2.0f;
	glLoadIdentity();
	glTranslatef(0.0f, -game->up, 0.0f);

	if (game->window.keyboard['R'].released) {
		add_player(game);
	}

	if (game->window.keyboard[KEY_ESC].released) {
		restart(game);
	}

	gfx_color(WHITE);
	draw_line_strip(game->left_wall, 64);
	draw_line_strip(game->right_wall, 64);
	FOR (i, 64) {
		gfx_color(RED);
		gfx_point(game->left_wall[i]);
	}

	vec2_t player_vel = {0};
	FORDYNARR (i, game->entities) {
		entity_t* entity = dynarr_get(&game->entities, i);

		if (!entity->dead) {
			entity->pos = add2(entity->pos, mul2f(entity->vel, game->dt * 10.0f));
			entity->rot += entity->spin * game->dt;
			entity->timer -= game->dt;

			if (DEBUG_MODE) {
				gfx_color(WHITE);
				gfx_point(entity->pos);
			}

			// Player with asteroids movement
			// Player shoots like asteroids
			if (entity->type == PLAYER) {
				update_player(game, entity);
				draw_player(entity);
				player_vel = entity->vel;
			}

			if (entity->type == BULLET) {
				glPointSize(3.0f);
				gfx_color(WHITE);
				gfx_point(entity->pos);
			}

			if (entity->type == DEBRIS) {
				glPushMatrix();
				glTranslatef(entity->pos.x, entity->pos.y, 0.0f);
				glRotatef(todeg(entity->rot), 0.0f, 0.0f, -1.0f);
				gfx_color(vec4(0.5f, 0.5f, 0.5f, 1.0f));
				gfx_line(entity->l1, entity->l2);
				glPopMatrix();

				if (entity->timer < 0.0f) {
					entity->dead = TRUE;
				}
			}

			if (entity->type == PARTICLE) {
				draw_glance_particle(*entity);
				if (entity->timer < 0.0f) {
					entity->dead = TRUE;
				}
			}

			// Obstacles are varied sizes
			// Obstacles differ in colour depending on their traits
			//		Red has a explosion radius when shot and destroys nearby obstacles
			//		Black is indestructable
			//		Green gets pushed when shot and destroys other obstacles on collision
			//		Blue is bouncy and you die if you shoot yourself
			// Obstacles kill the player on collision
			// Obstacles are randomly generated
			// Obstacles cover the screen left to right
			// Obstacles move downward slowly forcing the player to fly through them
			if (entity->type == OBSTACLE) {
				update_obstacle(game, entity);
				draw_obstacle(entity);
			}

			if (entity->type == BLAST) {
				entity->ani += 0.3f;
				gfx_color(RED);
				gfx_line_circle(entity->pos, entity->ani*2, 30);
				if (entity->ani > entity->collider.radius) {
					entity->dead = TRUE;
				}
			}

			if (entity->type == DEBUG_LINE) {
				gfx_color(entity->debug_line.color);
				gfx_line(entity->debug_line.l1, entity->debug_line.l2);
			}

			// Player and obstacles CANNOT go off the screen (invisible wall)
			// Bullets do not wrap around the screen
			// If all obstacles are off the bottom of the screen and the player is alive. Win!!

			FORDYNARR (j, game->entities) {
				if (i != j) {
					entity_t* entity2 = dynarr_get(&game->entities, j);
					entity_collision(game, entity, entity2);
					if (entity->type == PLAYER) {
						gfx_color(vec4(0.5f, 0.5f, 0.5f, 0.5f)); 
					}
				}
			}

			wall_collision(game, entity);

			// FOR (i, array_size(game->left_wall)-1) {
			// 	if (line_collision(game, entity, game->left_wall + i)) {
					
			// 	}
			// }
		}
	}

	// Resolve collisions
	int count = get_collision_count(game);
	FOR (i, array_size(game->collisions)) {
		if (game->collisions[i].e1) {
			FOR (j, array_size(game->collisions)) {
				if (game->collisions[j].e1 && i != j) {
					// Remove duplicate entity collisions
					collision_t* c1 = game->collisions + i;
					collision_t* c2 = game->collisions + j;
					if ((c1->e1==c2->e1 && c1->e2==c2->e2) ||
							(c1->e1==c2->e2 && c1->e2==c2->e1)) {
						game->collisions[j] = (collision_t){0};
					}
				}
			}
		}
	}

	FOR (i, array_size(game->collisions)) {
		collision_t* c = game->collisions + i;
		if (c->e1) {
			++game->collision_count_forever;
			if (c->e2) {
				entity_t e1 = *c->e1;
				entity_t e2 = *c->e2;
				entity_collision_effect(game, c->e1, e2);
				entity_collision_effect(game, c->e2, e1);
			}
			if (c->line) {
				// entity_collision_effect_wall(c->e1, c->line);
			}
		}
	}

	m_zero(game->collisions, sizeof(game->collisions));

	FORDYNARR (i, game->entities) {
		entity_t* entity = dynarr_get(&game->entities, i);
		if (entity->dead) {
			if (entity->type == OBSTACLE && entity->obstacle_type == ASTEROID_RED) {
				add_blast(game, entity->pos, entity->collider.radius*3.0f);
			}
			if (entity->type == PLAYER) {
				player_die(game, entity);
			}
			dynarr_pop(&game->entities, i);
			--i;
		}
	}

    // MAYBE LATER
    // Obstacles load from file

        // 128 px wide file. Infinite height

        // Obstacles loaded depending on the colour of the pixel

	glLoadIdentity();
	gfx_color(GREEN);
	gfx_texture(&game->font);
	gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 1), 4, "Collisions %i", game->collision_count_forever);
	gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 2), 4, "entities %i", game->entities.count);
	gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 3), 4, "speed %f", len2(player_vel));
	gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 4), 4, "up %f", game->up);
	gfx_texture(NULL);

	core_opengl_swap(&game->window);
}

void main() {
    void* arg = start();
    while (1) {
        frame(arg);
    }
}
