
b32 add_collision(game_t* game, collision_t* collision) {
	++game->collisions_this_frame;
	FOR (i, array_size(game->collisions)) {
		if (!game->collisions[i].e1) {
			game->collisions[i] = *collision;
			return TRUE;
		}
	}
	return FALSE;
}

int get_collision_count(game_t* game) {
	int result = 0;
	FOR (i, array_size(game->collisions)) {
		if (game->collisions[i].e1) {
			++result;
		}
	}
	return result;
}

//
// void add_wall_collision

b32 entity_point_collision(entity_t* entity, vec2_t p) {
	if (len2(sub2(entity->pos, p)) < entity->collider.radius) {
		return TRUE;
	}
	return FALSE;
}

void thing_off_the_internet(game_t* game, entity_t* a, entity_t b) {
	vec2_t normal = normalize2(sub2(b.pos, a->pos));
	vec2_t neg_normal = mul2f(normal, -1.0f);

	vec2_t apos = add2(b.pos, mul2f(neg_normal, (a->collider.radius+b.collider.radius)*1.1f));

	float aspeed = len2(a->vel);
	float bspeed = len2(b.vel);

	float mass_diff = b.mass / a->mass;

	vec2_t n1 = mul2(a->vel, normal);
	vec2_t n2 = mul2(b.vel, normal);
	float p = 1.0f * (n1.x + n1.y - n2.x - n2.y) / (a->mass + b.mass);

	a->vel = sub2(a->vel, mul2f(normal, p*b.mass));
	a->pos = apos;
}
 
void matts_physics(game_t* game, entity_t* a, entity_t b) {
	float dist = len2(sub2(a->pos, b.pos)) - (a->collider.radius + b.collider.radius);
	vec2_t normal = normalize2(sub2(b.pos, a->pos));
	vec2_t neg_normal = mul2f(normal, -1.0f);

	vec2_t apos = add2(a->pos, mul2f(normal, dist*1.0f));
	// vec2_t apos = add2(b.pos, mul2f(neg_normal, (a->collider.radius+b.collider.radius)*1.1f));
	// vec2_t apos = sub2(b.pos, a->pos);
	
	float aspeed = len2(a->vel);
	float bspeed = len2(b.vel);

	// float amul = 0.25f / a->mass;
	// float bmul = 0.25f / b.mass;
	float mass_diff = b.mass / a->mass * 0.1f;
	// float mass_diff = b.mass / a->mass;

	// float change = (bspeed / (a->mass/b.mass)) + (aspeed / (b.mass/a->mass));
	// vec2_t change = mul2f(sub2(b.vel, a->vel), b.mass/a->mass);
	
	// a->vel = (mul2f(a->vel, a->mass) + mul2f(b->vel, b->mass)) / m1 + m2
	a->vel = add2(a->vel, add2(mul2f(neg_normal, aspeed*mass_diff), mul2f(neg_normal, bspeed*mass_diff)));
	// a->vel = add2(a->vel, change);
	// a->vel = sub2(a->vel, mul2f(normal, p*b.mass));

	a->pos = apos;
}

void nath_physics(game_t* game, entity_t* a, entity_t b) {
    // Calculate the distance between the centers of the two circles
    float distance = sqrt(pow(a->pos.x - b.pos.x, 2) + pow(a->pos.y - b.pos.y, 2));
    
    // Calculate the unit normal vector pointing from b to a
    vec2_t normal = normalize2(sub2(a->pos, b.pos));
    
    // Calculate the unit tangent vector
    vec2_t tangent = vec2(-normal.y, normal.x);
    
    // Calculate the dot product of the velocity of a and the unit normal and tangent vectors
    float ndotva = dot2((a->vel), normal);
    float tdotva = dot2((a->vel), tangent);
	
    // Calculate the dot product of the velocity of b and the unit normal and tangent vectors
    float ndotvb = dot2((b.vel), normal);
    float tdotvb = dot2((b.vel), tangent);
    
    // Calculate the new normal velocities of a and b after the collision
    float newndotva = (ndotva * (a->mass - b.mass) + 2 * b.mass * ndotvb) / ((a->mass + b.mass)*2.0f); 
    
    // Calculate the new velocity vectors of a and b after the collision
    a->vel = add2(mul2f(normal, max(newndotva, 0.0f)), mul2f(tangent, tdotva));
	if (DEBUG_VELOCITY_LINES) {
		add_debug_line(game, a->pos, add2(a->pos, a->vel), vec4(1, 0, 1, 1));
	}
	
	if (len2(a->vel) > 3.0f) {
		int x = 0;
	}
    
    // Move entity a out of entity b to avoid overlapping
	if (b.type!=BULLET) {
		float overlap = (a->collider.radius + b.collider.radius) - distance;
		a->pos = add2(a->pos, mul2f(normal, max(overlap, 0.0f)));
	}
	
	if (len2(sub2(b.pos, add2(a->pos, a->vel))) < distance) {
		int x = 0;
	}
}

void attempt4(game_t* game, entity_t* a, entity_t b) {
	float dist = len2(sub2(a->pos, b.pos)) - (a->collider.radius+b.collider.radius);
	vec2_t normal = normalize2(sub2(a->pos, b.pos));
	vec2_t tangent = vec2(-normal.y, normal.x);

	float tdotva = dot2((a->vel), tangent);
	float tdotvb = dot2((b.vel), tangent);

	// a->vel = mul2f(tangent, tdotva);

	float overlap = dist;
    a->pos = add2(a->pos, mul2f(normal, overlap));
}

void entity_collision_effect(game_t* game, entity_t* a, entity_t b) {
	// Effect should only apply to the first entity,
	// the other entity will have its own function call.
	// Second param is a copy so we are working the previous
	// frame's data
	
	float dist = len2(sub2(a->pos, b.pos)) - (a->collider.radius + b.collider.radius);
	vec2_t normal = normalize2(sub2(b.pos, a->pos));
	vec2_t neg_normal = mul2f(normal, -1.0f);
	vec2_t vel = vec2(a->vel.x, -a->vel.y);

	if (a->type == PARTICLE || b.type == PARTICLE) {
		return;
	}

	if (a->type == PLAYER) {
		if (len2(a->vel) > 0.5f || a->on_bottom) {
			a->dead = TRUE;
			return;
		}
		else {
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.1f, vec4(1.0f, 1.0f, 0.2f, 1.0f)); 
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.1f, vec4(1.0f, 1.0f, 0.2f, 1.0f)); 
		}
		if (a->on_bottom) {
			a->dead = TRUE;
			return;
		}
	}

	if (a->type == OBSTACLE) {
		if (b.type == OBSTACLE) {
			// add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.2f, vec4(0.6f, 0.6f, 0.6f, 0.1f)); 
			// add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.2f, vec4(0.6f, 0.6f, 0.6f, 0.1f)); 
		}
		if (a->obstacle_type == ASTEROID_RED) {
			if (len2(b.vel) > 0.2f) {
				a->dead = TRUE;
			}
		}
	}

	if (b.type == BULLET) {
		if (a->type==OBSTACLE &&
		(a->obstacle_type==ASTEROID_RED ||
		a->obstacle_type==ASTEROID_WHITE)) {
			vec2_t vel = vec2(a->vel.x, -a->vel.y);
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.3f, vec4(1.0f, 1.0f, 0.2f, 1.0f)); 
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.3f, vec4(1.0f, 1.0f, 0.2f, 1.0f));
			if (DEBUG_MODE) {
				add_debug_line(game, a->pos, b.pos, PURPLE);
			}
			a->dead = TRUE;
		} else {
			play_sound(&game->audio, game->sounds.bullet, AUDIO_VOLUME);
		}
		// if (a->type==OBSTACLE && a->obstacle_type==ASTEROID_GREEN) {
		// 	a->dead = TRUE;
		// 	return;
		// }
		if (a->type == PLAYER) {
			a->dead = TRUE;
			return;
		}
	}

	if (a->type == BULLET) {
		if (b.type == OBSTACLE &&
			(b.obstacle_type == ASTEROID_BLUE /*||
			b.obstacle_type == ASTEROID_GREEN*/)) {
			a->vel = mul2f(neg_normal, len2(a->vel));
			a->pos = add2(a->pos, mul2f(normal, dist));
		} else {
			vec2_t vel = vec2(b.vel.x, -b.vel.y);
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.3f, vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
			add_glance_particles(game, add2(a->pos, mul2f(normal, a->collider.radius)), vel, 0.3f, vec4(1.0f, 1.0f, 1.0f, 1.0f));
			a->dead = TRUE;
		}
		return;
	}

	if (b.type != BULLET || a->type==OBSTACLE) {
		//(a->obstacle_type==ASTEROID_BLUE && a->obstacle_type==ASTEROID_GREEN)) {
		if (a->type == OBSTACLE && a->obstacle_type == ASTEROID_YELLOW && b.type == BULLET) {
			return;
		}
		nath_physics(game, a, b);
	}
}

void entity_collision_effect_wall(entity_t* entity, vec2_t* line) {
	vec2_t l1 = line[0];
	vec2_t l2 = line[1];
	vec2_t line_vector = normalize2(sub2(l2, l1));
	vec2_t normal = vec2(line_vector.y, -line_vector.x);

	// vec2_t aneg = normalize2(sub2(b->pos, a->pos));
	// vec2_t bneg = normalize2(sub2(a->pos, b->pos));

	float speed = len2(entity->vel);
	// float bspeed = len2(b->vel);
	float amul = 0.25f / entity->mass;
	// float bmul = 0.25f / b->mass;
	float mass_diff = 1.0f;//b->mass / a->mass * 0.1f;
	// float amul = 

	// if (a->type != BULLET) {
	// 	a->vel = add2(a->vel, mul2f(normal, speed*amul));
	// } else {
	// 	a->vel = mul2f(normal, speed);
	// }
	if (entity->type != BULLET) {
		entity->vel = add2(entity->vel, mul2f(normal, len2(entity->vel)));
	} else {
		entity->vel = mul2f(normal, len2(entity->vel));
	}
}

b32 line_collision(game_t* game, entity_t* entity, vec2_t* line) {
	vec2_t l1 = line[0];
	vec2_t l2 = line[1];

	if (entity_point_collision(entity, l1) ||
		entity_point_collision(entity, l2)) {
		return TRUE;
	}

	float len = len2(sub2(l1, l2));
	// float dot = dot2(l1, l2);
	// float dot = ((entity->pos.x-l1.x)*(l2.x-l1.x) + (entity->pos.y-l1.y)*(l2.y-l1.y)) / pow(len, 2);
	float dot = dot2(sub2(entity->pos, l1), sub2(l2, l1)) / pow(len, 2);
	vec2_t closest = add2(l1, mul2f(sub2(l2, l1), dot));

	// vec2_t c = cross
	vec2_t diff = sub2(entity->pos, closest);
	float angle = atan2f(diff.x, diff.y);

	if (/*angle > 0.0f &&*/ len2(sub2(closest, l1)) + len2(sub2(closest, l2)) <= len+1.0f) {
		// if (entity->type == PLAYER) {
			// gfx_line(entity->pos, closest);
		// }
		float dist = len2(diff); 
		if (dist < entity->collider.radius) {
			entity->pos = add2(entity->pos, mul2f(normalize2(diff), entity->collider.radius-dist));
			// entity->vel = vec2(0, 0);

			// if (len2(entity->vel) > 0.5f) {
			// 	if (entity->type == PLAYER) {
			// 		player_die(game, entity);
			// 	}
			// }
			// if (entity->type != BULLET) {
			// 	entity->vel = add2(entity->vel, mul2f(normalize2(diff), len2(entity->vel)));
			// } else {
			// 	entity->vel = mul2f(normalize2(diff), len2(entity->vel));
			// }

			// entity_collision_effect_wall(entity, normalize2(diff));
			collision_t collision = {0};
			collision.e1 = entity;
			collision.line = line;
			// collision.normal = 
			add_collision(game, &collision);

			return TRUE;
		}
	}

	return FALSE;
}

b32 entity_collision_test(entity_t* a, entity_t* b) {
	float radii = a->collider.radius+b->collider.radius;
	float dist = len2(sub2(a->pos, b->pos)) - radii;
	if (a->collider.type == COLLIDER_CIRCLE &&
		b->collider.type == COLLIDER_CIRCLE) {
		if (dist < 0.0f) {
			return TRUE;
		}
	}
	return FALSE;
}

b32 entity_collision(game_t* game, entity_t* a, entity_t* b) {
	float dist = len2(sub2(a->pos, b->pos)) - (a->collider.radius + b->collider.radius);

	if (a->collider.type == COLLIDER_CIRCLE &&
		b->collider.type == COLLIDER_CIRCLE) {
		if (dist < 0.0f) {
			

			// entity_collision_effect(a, b);
			// entity_collision_effect(b, a);
			collision_t collision = {0};
			collision.e1 = a;
			collision.e2 = b;
			add_collision(game, &collision);
			// entity_collision_effect(game, a, b);

			// a->vel = add2(a->vel, mul2f(sub2(b->vel, a->vel), 0.5f));
			// b->vel = add2(b->vel, mul2f(sub2(a->vel, b->vel), 0.5f));

			return TRUE;
		}
	}

	if (a->collider.type == COLLIDER_CIRCLE &&
		b->collider.type == COLLIDER_DEATH) {
		float dist = len2(sub2(a->pos, b->pos)) - (a->collider.radius + b->ani);
		if (dist < 0.0f && (a->type != OBSTACLE || a->obstacle_type != ASTEROID_YELLOW)) {
			a->dead = TRUE;
		}
	}

	return FALSE;
}

b32 screen_edge_collision(game_t* game, entity_t* entity) {
	if (entity->collider.type == COLLIDER_CIRCLE) {
		if (entity->pos.x-entity->collider.radius < SCREEN_LEFT) {
			entity->pos.x = SCREEN_LEFT+entity->collider.radius;
			entity->vel.x = -entity->vel.x;
		}
		if (entity->pos.x+entity->collider.radius > SCREEN_RIGHT) {
			entity->pos.x = SCREEN_RIGHT-entity->collider.radius;
			entity->vel.x = -entity->vel.x;
		}
		if (entity->type==PLAYER && entity->pos.y-entity->collider.radius-game->up < SCREEN_BOTTOM) {
			entity->pos.y = SCREEN_BOTTOM+game->up+entity->collider.radius + 0.1f;
			entity->vel.y = entity->vel.y + 0.1f;
			entity->on_bottom = TRUE;
		} else {
			entity->on_bottom = FALSE;
		}
		if (entity->type==OBSTACLE &&
			entity->obstacle_type!=ASTEROID_WHITE) {
			if (entity->pos.y+entity->collider.radius > WALL_NARROW_START) {
				entity->pos.y = WALL_NARROW_START-entity->collider.radius;
				entity->vel.y = -entity->vel.y;
			}
		}
	}
}

b32 wall_collision(game_t* game, entity_t* entity) {
	int wall_segment = entity->pos.y + SCREEN_TOP;
	wall_segment = imax(wall_segment, 0);

	if (entity->collider.type == COLLIDER_CIRCLE) {
		if (wall_segment < array_size(game->left_wall)-1) {
			{
				vec2_t* segment = game->left_wall + wall_segment;

				float intersect = entity->pos.y - (segment)->y;
				vec2_t point = lerp2(segment[0], segment[1], intersect);

				if (entity->pos.x - entity->collider.radius < point.x) {
					if (len2(entity->vel) > 0.5f) {
						entity->dead = TRUE;
						return TRUE;
					}

					entity->pos.x = point.x + entity->collider.radius;

					vec2_t tangent = normalize2(sub2(segment[0], segment[1]));
					vec2_t normal = vec2(-tangent.y, tangent.x);

					float ndotva = dot2(normal, entity->vel);
					float ndotvb = dot2(normal, vec2(0, 0));
					float tdotva = dot2(tangent, entity->vel);
					float amass = entity->mass;
					float bmass = 100;
					float newndotva = (ndotva * (amass-bmass) + 2 * bmass * ndotvb) / ((amass + bmass)*2.0f);
					entity->vel = add2(mul2f(normal, max(newndotva, 0.0f)), mul2f(tangent, tdotva));

					return TRUE;
				}

				// gfx_color(vec4(1, 0, 1, 1));
				// gfx_line(game->left_wall[wall_segment], game->left_wall[wall_segment+1]);
				// gfx_point(point);
			}

			{
				vec2_t* segment = game->right_wall + wall_segment;

				float intersect = entity->pos.y - (segment)->y;
				vec2_t point = lerp2(segment[0], segment[1], intersect);

				if (entity->pos.x + entity->collider.radius > point.x) {
					if (len2(entity->vel) > 0.5f) {
						entity->dead = TRUE;
						return TRUE;
					}

					entity->pos.x = point.x - entity->collider.radius;

					vec2_t tangent = normalize2(sub2(segment[1], segment[0]));
					vec2_t normal = vec2(-tangent.y, tangent.x);

					float ndotva = dot2(normal, entity->vel);
					float ndotvb = dot2(normal, vec2(0, 0));
					float tdotva = dot2(tangent, entity->vel);
					float amass = entity->mass;
					float bmass = 10;
					float newndotva = (ndotva * (amass-bmass) + 2 * bmass * ndotvb) / ((amass + bmass)*2.0f);
					entity->vel = add2(mul2f(normal, max(newndotva, 0.0f)), mul2f(tangent, tdotva));

					return TRUE;
				}

				// gfx_color(vec4(1, 0, 1, 1));
				// gfx_line(game->left_wall[wall_segment], game->left_wall[wall_segment+1]);
				// gfx_point(point);
			}
		}
	}

	return FALSE;
}
