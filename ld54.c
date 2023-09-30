
#include <platform.h>
#include <im.h>
#include <timer.h>

#define SCREEN_LEFT -1280.0f/64.0f
#define SCREEN_RIGHT 1280.0f/64.0f
#define SCREEN_BOTTOM -720.0f/64.0f
#define SCREEN_TOP 720.0f/64.0f
#define SCREEN_WIDTH (SCREEN_RIGHT - SCREEN_LEFT)
#define SCREEN_HEIGHT (SCREEN_TOP - SCREEN_BOTTOM)

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
};
typedef enum {
	ASTEROID_RED,
	ASTEROID_GREEN,
	ASTEROID_BLUE,

	ASTEROID_MAX,
} asteroid_type_t;

typedef enum {
	PLAYER = 1,
	OBSTACLE = 2,
	BULLET = 3,
	DEBRIS = 4,
} entity_type_t;

typedef struct {
	enum {
		COLLIDER_NONE,
		COLLIDER_CIRCLE,
		COLLIDER_POINT,
	} type;
	float radius;
} collider_t;

// typedef struct {
//     vec2_t pos;
//     float w;
//     float h;
//     vec4_t colour;
// } obstacle_t;
//
// typedef struct {
// 	vec2_t pos;
// 	vec2_t vel;
// 	float rot;
// 	float ani;
// } player_t;
//
// typedef struct {
// 	
// } bullet_t;

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
		// Obstacle properties
		struct {
			asteroid_type_t obstacle_type;
			vec4_t color;
		};

		// Debris properties
		struct {
			vec2_t l1;
			vec2_t l2;
		};
	};

} entity_t;

typedef struct {
	core_window_t window;
	core_timer_t timer;
	float dt;

	// entity_t player;
	dynarr_t entities;

	vec2_t left_wall[64];
	vec2_t right_wall[64];

} game_t;

entity_t create_obstacle(vec2_t pos, float size, vec4_t colour) {
    entity_t obstacle = {0};
	obstacle.type = OBSTACLE;
	obstacle.obstacle_type = r_int_range(0, ASTEROID_MAX);
    obstacle.pos = pos;
	obstacle.collider.radius = r_float_range(2.0f, 1.0f);
	obstacle.color = colour;
    // obstacle.w = size.x;
    // obstacle.h = size.y;
	obstacle.collider.radius = size / 2.0f;
	obstacle.collider.type = COLLIDER_CIRCLE;
    return obstacle;
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

void initialise(game_t* game) {
	game->entities = dynarr(sizeof(entity_t));

	add_player(game);
	
    entity_t obstacle;
	FOR (i, 10) {
		obstacle = create_obstacle(
			vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT),
				r_float_range(SCREEN_BOTTOM, SCREEN_TOP)),
			2,
			vec4(1, 1, 1, 1));
		dynarr_push(&game->entities, &obstacle);
	}
    //
    // obstacle = create_obstacle(vec2(1, 0), 5, vec4(1, 0, 0, 1));
    // dynarr_push(&game->entities, &obstacle);
    // 
    // obstacle = create_obstacle(vec2(2, 0), 2, vec4(1, 0, 1, 1));
    // dynarr_push(&game->entities, &obstacle);
    //
    // obstacle = create_obstacle(vec2(3, 0), 3, vec4(0, 0, 1, 1));
    // dynarr_push(&game->entities, &obstacle);
	
	FOR (i, 64) {
		game->left_wall[i] = vec2(
				r_float_range(
					SCREEN_LEFT + 1, SCREEN_LEFT + 2),
					SCREEN_BOTTOM + (f32)i);
	}
}

void* start() {
	game_t* game = malloc(sizeof(game_t));
	m_zero(game, sizeof(game_t));
	core_window(&game->window, "LD54", 1280, 720, 0);
	core_opengl(&game->window);
	core_timer(&game->timer);

		// game->obstacles = game->obstacles.arena.address;
	initialise(game);
	
	return game;
}

void update_obstacle(game_t* game, entity_t* obstacle) {
	obstacle->mass = 5;
}

void draw_obstacle(entity_t* obstacle) {
	gfx_color(obstacle_colors[obstacle->obstacle_type]);
	gfx_line_circle(obstacle->pos, obstacle->collider.radius * 2.0f, 30);
}

void update_player(game_t* game, entity_t* player) {
	player->mass = 1;

	core_button_t* keyboard = game->window.keyboard;
	if (keyboard[KEY_UP].down) {
		player->vel = add2(player->vel, mul2f(rotate2(UP, player->rot), game->dt*1.0f));
		player->ani += game->dt * 10.0f;
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

	// float speed = len2(player->vel);
	// if (speed > 0.01f) {
		// vec2_t dir = normalize2(player->vel);
		// player->vel = sub2(player->vel, mul2f(dir, game->dt*0.2f));
	// }
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
	glPopMatrix();
}

b32 entity_point_collision(entity_t* entity, vec2_t p) {
	if (len2(sub2(entity->pos, p)) < entity->collider.radius) {
		return TRUE;
	}
	return FALSE;
}

b32 line_collision(entity_t* entity, vec2_t l1, vec2_t l2) {
	if (entity_point_collision(entity, l1) ||
		entity_point_collision(entity, l2)) {
		return TRUE;
	}

	float len = len2(sub2(l1, l2));
	// float dot = dot2(l1, l2);
	// float dot = ((entity->pos.x-l1.x)*(l2.x-l1.x) + (entity->pos.y-l1.y)*(l2.y-l1.y)) / pow(len, 2);
	float dot = dot2(sub2(entity->pos, l1), sub2(l2, l1)) / pow(len, 2);
	vec2_t closest = add2(l1, mul2f(sub2(l2, l1), dot));
	gfx_line(entity->pos, closest);
}

void entity_collision_effect(entity_t* a, entity_t* b) {
	vec2_t aneg = normalize2(sub2(b->pos, a->pos));
	vec2_t bneg = normalize2(sub2(a->pos, b->pos));

	float aspeed = len2(a->vel);
	float bspeed = len2(b->vel);
	float amul = 0.25f / a->mass;
	float bmul = 0.25f / b->mass;
	float mass_diff = b->mass / a->mass * 0.1f;
	// float amul = 

	// a->vel = (mul2f(a->vel, a->mass) + mul2f(b->vel, b->mass)) / m1 + m2
	if (a->type != BULLET) {
		a->vel = add2(a->vel, add2(mul2f(bneg, aspeed*amul), mul2f(bneg, bspeed*amul)));
	} else {
		a->vel = mul2f(bneg, aspeed);
	}
	// b->vel = add2(b->vel, add2(mul2f(aneg, bspeed*bmul), mul2f(aneg, aspeed*bmul)));
}

b32 entity_collision(game_t* game, entity_t* a, entity_t* b) {
	if (a->collider.type == COLLIDER_CIRCLE &&
		b->collider.type == COLLIDER_CIRCLE) {
		float dist = len2(sub2(a->pos, b->pos)) - (a->collider.radius + b->collider.radius);
		if (dist < 0.0f) {
			vec2_t aneg = normalize2(sub2(b->pos, a->pos));
			vec2_t bneg = normalize2(sub2(a->pos, b->pos));
			vec2_t apos = add2(a->pos, mul2f(normalize2(sub2(b->pos, a->pos)), dist/2));
			vec2_t bpos = add2(b->pos, mul2f(normalize2(sub2(a->pos, b->pos)), dist/2));

			entity_collision_effect(a, b);
			entity_collision_effect(b, a);

			// a->vel = add2(a->vel, mul2f(sub2(b->vel, a->vel), 0.5f));
			// b->vel = add2(b->vel, mul2f(sub2(a->vel, b->vel), 0.5f));

			a->pos = apos;
			// a->vel = avel;
			b->pos = bpos;

			if (a->type == PLAYER) {
				FOR (i, array_size(player_model)) {
					add_debris(game, a->vel, add2(a->pos, rotate2(player_model[i], a->rot)), add2(a->pos, rotate2(player_model[i+1], a->rot)));
				}
				a->dead = TRUE;
			}

			return TRUE;
		}
	}
	return FALSE;
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

	if (game->window.keyboard['R'].released) {
		add_player(game);
	}

	// gfx_color(WHITE);
	draw_line_strip(game->left_wall, 64);

	FORDYNARR (i, game->entities) {
		entity_t* entity = dynarr_get(&game->entities, i);

		if (TRUE) {
			entity->pos = add2(entity->pos, mul2f(entity->vel, game->dt * 10.0f));
			entity->rot += entity->spin * game->dt;
			entity->timer -= game->dt;

			// Player with asteroids movement
			// Player shoots like asteroids
			if (entity->type == PLAYER) {
				update_player(game, entity);
				draw_player(entity);

				line_collision(entity, game->left_wall[0], game->left_wall[1]);
			}

			if (entity->type == BULLET) {
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

			// Obstacles are varied sizes
			// Obstacles differ in colour depending on their traits
			// Red has a explosion radius when shot and destroys nearby obstacles
			// Black is indestructable
			// Green gets pushed when shot and destroys other obstacles on collision
			// Blue is bouncy and you die if you shoot yourself
			// Obstacles kill the player on collision
			// Obstacles are randomly generated
			// Obstacles cover the screen left to right
			// Obstacles move downward slowly forcing the player to fly through them
			if (entity->type == OBSTACLE) {
				update_obstacle(game, entity);
				draw_obstacle(entity);
			}

			// Player and obstacles CANNOT go off the screen (invisible wall)
			// Bullets do not wrap around the screen
			// If all obstacles are off the bottom of the screen and the player is alive. Win!!

			FORDYNARR (j, game->entities) {
				if (i != j) {
					entity_t* entity2 = dynarr_get(&game->entities, j);
					entity_collision(game, entity, entity2);
				}
			}

			if (entity->pos.x < SCREEN_LEFT) entity->pos.x = SCREEN_RIGHT;
			if (entity->pos.x > SCREEN_RIGHT) entity->pos.x = SCREEN_LEFT;
			if (entity->pos.y < SCREEN_BOTTOM) entity->pos.y = SCREEN_TOP;
			if (entity->pos.y > SCREEN_TOP) entity->pos.y = SCREEN_BOTTOM;
		}
	}

	FORDYNARR (i, game->entities) {
		entity_t* entity = dynarr_get(&game->entities, i);
		if (entity->dead) {
			dynarr_pop(&game->entities, i);
		}
	}


    // MAYBE LATER
    // Obstacles load from file

        // 128 px wide file. Infinite height

        // Obstacles loaded depending on the colour of the pixel

	core_opengl_swap(&game->window);
}

// void main() {
//     void* arg = start();
//     while (1) {
//         frame(arg);
//     }
// }
