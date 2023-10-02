// TODO
// Change line collision algorithm

// #include <windows.h>
// #include <mmeapi.h>

#include <core/platform.h>
#include <core/im.h>
#include <core/timer.h>
#include <core/file.h>

#define AUDIO_VOLUME_ENGINE (0.05f)
#define AUDIO_VOLUME (0.25f)
#define AUDIO_ON TRUE
// #define AUDIO_WAVEOUT TRUE

#ifdef AUDIO_ON
#include <core/audio.h>

void init_audio(core_audio_t* audio) {
	core_init_audio(audio, CORE_DEFAULT_AUDIO_MIXER_PROC, 0);
}

// void play_sound(core_audio_t* audio, audio_buffer_t* buffer) {
// 	core_play_sound(audio, buffer, 0.5f);
// }
#define play_sound(core_audio, audio_buffer, vol)\
	core_play_sound(core_audio, audio_buffer, vol)
#else
// #define play_sound
#endif

#ifdef AUDIO_WAVEOUT
#include <audio.h>

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT umsg, DWORD_PTR dwinstance, DWORD_PTR dwparam, DWORD_PTR dwparam2) {
	printf("wave out callback \n");
}

void init_waveout(core_audio_t* audio) {
	// HWAVEOUT hwo;
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 88125;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec*2*2;
	wfx.nBlockAlign = 2*2;
	wfx.wBitsPerSample = 16;
	wfx.cbSize = 0;

	if (waveOutOpen(&audio->hwaveout, WAVE_MAPPER, &wfx, waveOutProc, 0, CALLBACK_FUNCTION)
		!= MMSYSERR_NOERROR) {
		printf("waveOutOpen failed \n");
		// return FALSE;
	}
}

void play_sound(core_audio_t* audio, audio_buffer_t* buffer, float vol) {
	WAVEHDR wave_header = {0};
	wave_header.lpData = buffer->data;
	wave_header.dwBufferLength = buffer->sample_count * buffer->bytes_per_sample;
	wave_header.dwFlags = 0;

	if (waveOutPrepareHeader(audio->hwaveout, &wave_header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		printf("Failed to preare wave header \n");
	}
	if (waveOutWrite(audio->hwaveout, &wave_header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		printf("Failed to write wave out \n");
	}
}
#endif

#define DEBUG_MODE FALSE
#define DEBUG_TEXT FALSE
#define DEBUG_VELOCITY_LINES FALSE

#define SCREEN_LEFT -1280.0f/64.0f
#define SCREEN_RIGHT 1280.0f/64.0f
#define SCREEN_BOTTOM -720.0f/64.0f
#define SCREEN_TOP 720.0f/64.0f
#define SCREEN_WIDTH (SCREEN_RIGHT - SCREEN_LEFT)
#define SCREEN_HEIGHT (SCREEN_TOP - SCREEN_BOTTOM)

#define OBSTACLE_VARIENCE 0.25f

#define WALL_STEP 1.0f
#define WALL_SEGMENT_COUNT 256
#define WALL_BLANK_END (SCREEN_TOP*2)
#define WALL_LEVEL_START (WALL_BLANK_END+8)
#define WALL_NARROW_START (WALL_LEVEL_START + 64)
#define WALL_TUNNEL_START (WALL_NARROW_START + 16)
#define WALL_FINISH_START (WALL_TUNNEL_START + 16)
#define WALL_FINISH_END (WALL_FINISH_START + 16)

#define WHITE (vec4_t){1, 1, 1, 1}
#define RED (vec4_t){1, 0, 0, 1}
#define GREEN (vec4_t){0, 1, 0, 1}
#define BLUE (vec4_t){0, 0, 1, 1}
#define PURPLE (vec4_t){1, 0, 1, 1}
#define YELLOW (vec4_t){1, 1, 0, 1}

vec2_t UP = {0.0f, 1.2f};

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
	YELLOW,
};
typedef enum {
	ASTEROID_RED,
	ASTEROID_GREEN,
	ASTEROID_BLUE,
	ASTEROID_WHITE,
	ASTEROID_YELLOW,
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
	vec4_t color;
	int life;
	b32 dead;
	b32 on_bottom;

	union {
		// Player properties
		// struct {
		// };

		// Obstacle properties
		struct {
			asteroid_type_t obstacle_type;
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
#ifdef AUDIO_ON
	core_audio_t audio;
#endif
#ifdef AUDIO_WAVEOUT
	core_audio_t audio;
#endif
	float dt;

	dynarr_t entities;
	collision_t collisions[128];
	int collision_count_forever;

	float up;
	b32 pause;
	int level;
	b32 death_screen;
	float death_countdown;
	int level_loop;
	int most_collisions_in_a_frame;
	int collisions_this_frame;

	vec2_t left_wall[WALL_SEGMENT_COUNT];
	vec2_t right_wall[WALL_SEGMENT_COUNT];

	m_arena assets;
	gfx_texture_t font;

#ifdef AUDIO_ON
	struct {
		audio_buffer_t* shoot;
		audio_buffer_t* death;
		audio_buffer_t* explosion;
		audio_buffer_t* bullet;
		audio_buffer_t* engine;
	} sounds;
#endif
#ifdef AUDIO_WAVEOUT
	struct {
		audio_buffer_t* impact;
		audio_buffer_t* shoot;
		audio_buffer_t* death;
		audio_buffer_t* explosion;
		audio_buffer_t* bullet;
		audio_buffer_t* engine;
		audio_buffer_t* engine2;
		audio_buffer_t* engine3;
	} sounds;
#endif

} game_t;

typedef struct {
	int level;
	int level_loop;
} save_t;

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

void add_player(game_t* game, vec2_t pos) {
	entity_t player = {0};
	player.type = PLAYER;
	player.pos = pos;
	player.collider.type = COLLIDER_CIRCLE;
	player.collider.radius = 0.4f;
	dynarr_push(&game->entities, &player);
}

void add_existing_player(game_t* game, entity_t player) {
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
	FOR (i, array_size(player_model)-1) {
		add_debris(game, player->vel, add2(player->pos, rotate2(player_model[i], player->rot)), add2(player->pos, rotate2(player_model[i+1], player->rot)));
	}
	player->dead = TRUE;
}

void add_debug_line(game_t* game, vec2_t l1, vec2_t l2, vec4_t color) {
	entity_t line = {0};
	line.type = DEBUG_LINE;
	line.debug_line.l1 = l1;
	line.debug_line.l2 = l2;
	line.color = color;
	dynarr_push(&game->entities, &line);
}


#include "collision.c"
#include "level.c"


void generate_walls(game_t* game) {
	FOR (i, WALL_SEGMENT_COUNT) {
		float y = SCREEN_BOTTOM + (f32)i * WALL_STEP;
		float x = SCREEN_LEFT + 3;
		float x2 = SCREEN_RIGHT - 3;

		float intro_x = SCREEN_LEFT;
		float level_x = SCREEN_LEFT+3;
		float tunnel_x = -3;
		float end_x = SCREEN_LEFT;
		
		if (i > WALL_FINISH_START) {
			// Finish section
			float t = (f32)(i-WALL_FINISH_START)/
				(WALL_FINISH_END-WALL_FINISH_START);
			float wx = lerp(tunnel_x, end_x, t);
			float wx2 = lerp(tunnel_x, end_x, t) * -1;
			game->left_wall[i] = vec2(r_float_range(wx, wx+1), y);
			game->right_wall[i] = vec2(r_float_range(wx2, wx2-1), y);
		} else if (i > WALL_TUNNEL_START) {
			// Tunnel
			float wx = tunnel_x;
			float wx2 = tunnel_x * -1;
			game->left_wall[i] = vec2(r_float_range(wx, wx+1), y);
			game->right_wall[i] = vec2(r_float_range(wx2, wx2-1), y);
		} else if (i > WALL_NARROW_START) {
			// Narrowing before tunnel
			float t = (f32)(i-WALL_NARROW_START)/
				(WALL_TUNNEL_START-WALL_NARROW_START);
			float wx = lerp(level_x, tunnel_x, t);
			float wx2 = lerp(level_x, tunnel_x, t) * -1;
			game->left_wall[i] = vec2(r_float_range(wx, wx+1), y);
			game->right_wall[i] = vec2(r_float_range(wx2, wx2-1), y);
		} else if (i > WALL_LEVEL_START) {
			// Main level
			game->left_wall[i] = vec2(
					r_float_range(
						x, x+1),
						y);
			game->right_wall[i] = vec2(
					r_float_range(
						x2, x2-1),
						y);
		} else if (i > WALL_BLANK_END) {
			// Intro section
			// float t = (f32)i/WALL_LEVEL_START;
			float t = (f32)(i-WALL_BLANK_END)/
				(WALL_LEVEL_START-WALL_BLANK_END);
			float wx = lerp(intro_x, level_x, t);
			float wx2 = lerp(intro_x, level_x, t) * -1;
			game->left_wall[i] = vec2(r_float_range(wx, wx+1), y);
			game->right_wall[i] = vec2(r_float_range(wx2, wx2-1), y);
		} else {
			game->left_wall[i] = vec2(SCREEN_LEFT-1, y);
			game->right_wall[i] = vec2(SCREEN_RIGHT+1, y);
		}
	}
}

void load_level(game_t* game, vec2_t player_pos) {
	// FOR (i, game->entities.count) {
	// 	dynarr_pop(&game->entities, game->entities.count-1);
	// }
	VirtualFree(game->entities.arena.address,
		game->entities.arena.size, MEM_RELEASE);
	game->entities = dynarr(sizeof(entity_t));
	
	add_player(game, player_pos);
	levels[game->level](game);
	generate_walls(game);
	game->up = 0.0f;

	save_t save = {
		game->level,
		game->level_loop,
	};
	f_handle save_file = f_open("save.dat");
	f_info info = f_stat(save_file);
	f_write(save_file, 0, &save, sizeof(save_t));
	f_close(save_file);
}

void load_next_level(game_t* game, entity_t player) {
	// FOR (i, game->entities.count) {
	// 	dynarr_pop(&game->entities, game->entities.count-1);
	// }
	VirtualFree(game->entities.arena.address,
		game->entities.arena.size, MEM_RELEASE);
	game->entities = dynarr(sizeof(entity_t));
	
	add_existing_player(game, player);
	levels[game->level](game);
	generate_walls(game);

	save_t save = {
		game->level,
		game->level_loop,
	};
	f_handle save_file = f_open("save.dat");
	f_info info = f_stat(save_file);
	f_write(save_file, 0, &save, sizeof(save_t));
	f_close(save_file);
}

void initialise(game_t* game) {
	game->entities = dynarr(sizeof(entity_t));

	m_stack(&game->assets, 0 ,0);
	m_reserve(&game->assets, GB(1), PAGE_SIZE);
	bitmap_t* fontbmp = f_load_font_file(&game->assets, "font.bmp");
	game->font = gfx_create_texture(fontbmp);

	load_level(game, vec2(0, 0));
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
	core_window(&game->window, "Limited in Space", 1280, 720, 0);
	core_opengl(&game->window);
	core_timer(&game->timer);
#ifdef AUDIO_ON
	init_audio(&game->audio);
#endif
#ifdef AUDIO_WAVEOUT
	init_waveout(&game->audio);
#endif

	save_t save;
	f_handle save_file = f_open("save.dat");
	f_info info = f_stat(save_file);
	if (info.size >= sizeof(save_t)) {
		f_read(save_file, 0, &save, sizeof(save_t));
		game->level = save.level;
		game->level_loop = save.level_loop;
	}
	f_close(save_file);

	s_pool(&game->assets);
	initialise(game);

#ifdef AUDIO_ON
	game->sounds.shoot = f_load_wave(&game->assets, "shoot.wav");
	game->sounds.death = f_load_wave(&game->assets, "death.wav");
	game->sounds.explosion = f_load_wave(&game->assets, "explosion.wav");
	game->sounds.bullet = f_load_wave(&game->assets, "bullet.wav");
	game->sounds.engine = f_load_wave(&game->assets, "engine.wav");
#endif
#ifdef AUDIO_WAVEOUT
	game->sounds.impact = f_load_wave(&game->assets, "impact.wav");
	game->sounds.shoot = f_load_wave(&game->assets, "shoot.wav");
	game->sounds.death = f_load_wave(&game->assets, "death.wav");
	game->sounds.explosion = f_load_wave(&game->assets, "explosion.wav");
	game->sounds.bullet = f_load_wave(&game->assets, "bullet.wav");
	game->sounds.engine = f_load_wave(&game->assets, "engine.wav");
	game->sounds.engine2 = f_load_wave(&game->assets, "engine2.wav");
	game->sounds.engine3 = f_load_wave(&game->assets, "engine3.wav");
#endif
	
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
		player->timer -= game->dt;
		if (player->timer < 0.0f) {
			play_sound(&game->audio, game->sounds.engine, AUDIO_VOLUME_ENGINE);
			player->timer = 0.2f;
		}
	} else {
		player->ani = 0;
	}
	if (keyboard[KEY_LEFT].down) {
		player->rot -= game->dt * 5.0f;
	}
	if (keyboard[KEY_RIGHT].down) {
		player->rot += game->dt * 5.0f;
	}

	if (keyboard[KEY_SPACE].pressed) {
		add_bullet(game, add2(player->pos, rotate2(player_model[1], player->rot)), player->rot);
#ifdef AUDIO_ON
		play_sound(&game->audio, game->sounds.shoot, AUDIO_VOLUME);
#endif
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
#ifdef AUDIO_ON
	if (game->audio.reload) {
		game->audio.reload = FALSE;
		core_init_audio(&game->audio, CORE_DEFAULT_AUDIO_MIXER_PROC, 0);
	}
#endif
	core_window_update(&game->window);
	core_timer_update(&game->timer);
	game->dt = game->timer.dt;
    
	if (DEBUG_MODE) {
		if (game->window.keyboard['R'].released) {
			add_player(game, vec2(0, 0));
		}
		if (game->window.keyboard['P'].released) {
			game->pause = !game->pause;
		}
		if (game->window.keyboard[KEY_ESC].released) {
			restart(game);
		}
		if (game->window.keyboard[KEY_F2].released) {
			++game->level_loop;
		}
		if (game->window.keyboard[KEY_F1].released) {
			game->level_loop = imax(game->level_loop-1, 0);
		}
	}

	gfx_clear(vec4(0, 0, 0.0f, 1));
	gfx_coord_system(1280.0f/64.0f, 720.0f/64.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1280.0f/64.0f, 1280.0f/64.0f,
			-720.0f/64.0f, 720.0f/64.0f,
			-10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPointSize(3.0f);

	game->collisions_this_frame = 0;
	if (!game->pause && !game->death_screen) {
		game->up += game->dt * (3.0f + game->level_loop);
	}
	glLoadIdentity();
	glTranslatef(0.0f, -game->up, 0.0f);

	gfx_color(WHITE);
	draw_line_strip(game->left_wall, WALL_SEGMENT_COUNT);
	draw_line_strip(game->right_wall, WALL_SEGMENT_COUNT);
	FOR (i, WALL_SEGMENT_COUNT) {
		gfx_color(RED);
		// gfx_point(game->left_wall[i]);
	}

	vec2_t player_pos = {0};
	vec2_t player_vel = {0};
	entity_t current_player;
	b32 next_level = FALSE;
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

				if (game->up > WALL_FINISH_END) {
					entity->pos.y = entity->pos.y - game->up;
					game->up = 0.0f;
					
					++game->level;
					game->level %= array_size(levels);
					if (game->level == 0) {
						game->level_loop++;
					}
					next_level = TRUE;
				}

				player_pos = entity->pos;
				player_vel = entity->vel;
				current_player = *entity;
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
				gfx_color(entity->color);
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
			screen_edge_collision(game, entity);
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
			// if (c->line) {
			// 	// entity_collision_effect_wall(c->e1, c->line);
			// }
		}
	}

	m_zero(game->collisions, sizeof(game->collisions));

	FORDYNARR (i, game->entities) {
		entity_t* entity = dynarr_get(&game->entities, i);
		if (entity->dead) {
			if (entity->type == OBSTACLE) {
				if (entity->obstacle_type == ASTEROID_RED) {
					add_blast(game, entity->pos, entity->collider.radius*3.0f);
					play_sound(&game->audio, game->sounds.explosion, AUDIO_VOLUME);
				}
				play_sound(&game->audio, game->sounds.death, AUDIO_VOLUME);
			}
			if (entity->type == PLAYER) {
				player_die(game, entity);
				play_sound(&game->audio, game->sounds.explosion, AUDIO_VOLUME);
				game->death_screen = TRUE;
				game->death_countdown = 3.0f;
			}
			dynarr_pop(&game->entities, i);
			--i;
		}
	}

	if (next_level) {
		load_next_level(game, current_player);
	}

    // MAYBE LATER
    // Obstacles load from file

        // 128 px wide file. Infinite height

        // Obstacles loaded depending on the colour of the pixel

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gfx_color(GREEN);
	gfx_texture(&game->font);

	if (game->death_screen) {
		game->death_countdown -= game->dt;
		if (game->death_countdown < 0.0f) {
			game->death_screen = FALSE;
			load_level(game, vec2(0, 0));
		}

		gfx_text(&game->window, vec2(-2, game->up + 3), 4, "You Died");
	}
	
	char buffer[64] = {0};
	FOR (i, game->level_loop) {
		buffer[i] = '+';
	}
	gfx_text(&game->window, vec2(-1.5f, 3), 4, "Level %i%s", game->level+1, buffer);

	gfx_text(&game->window, vec2(-3, WALL_FINISH_END-SCREEN_TOP), 4, "Level Complete");
	gfx_text(&game->window, vec2(-1.5f, WALL_FINISH_END+3), 4, "Level %i%s", ((game->level+1)%array_size(levels))+1, buffer);

	if (game->collisions_this_frame > game->most_collisions_in_a_frame) {
		game->most_collisions_in_a_frame = game->collisions_this_frame;
	}

	glLoadIdentity();
	gfx_color(GREEN);
	if (DEBUG_TEXT) {
		gfx_texture(&game->font);
		gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 1), 4, "Collisions %i", game->collision_count_forever);
		gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 2), 4, "Most collisions in a frame %i/%i", game->most_collisions_in_a_frame, array_size(game->collisions));
		gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 3), 4, "entities %i", game->entities.count);
		// gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 3), 4, "speed %f", len2(player_vel));
		// gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 4), 4, "up %f", game->up);
		gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 5), 4, "level %i", game->level+1);
		gfx_text(&game->window, vec2(SCREEN_LEFT+1, SCREEN_TOP - 6), 4, "level loop %i", game->level_loop);
	}
	gfx_texture(NULL);

	core_opengl_swap(&game->window);
}

void main() {
    void* arg = start();
    while (1) {
        frame(arg);
    }
}
