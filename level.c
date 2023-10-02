
typedef void (*level_proc_t)(game_t* game);



entity_t create_obstacle(game_t* game, int type, float size) {
	entity_t obstacle = {0};
	float radius = size / 2.0f;

	FOR (i, 130) {
		obstacle.type = OBSTACLE;
		obstacle.pos = vec2(r_float_range(SCREEN_LEFT, SCREEN_RIGHT),
						r_float_range(SCREEN_TOP, WALL_NARROW_START+SCREEN_TOP));
		obstacle.obstacle_type = type;
		obstacle.collider.radius = radius;
		obstacle.collider.type = COLLIDER_CIRCLE;

		b32 collision = FALSE;
		FORDYNARR (other, game->entities) {
			entity_t* other_obstacle = dynarr_get(&game->entities, other);
			if (entity_collision_test(&obstacle, other_obstacle)) {
				collision = TRUE;
				break;
			}
		}
		if (!collision) {
			break;
		}
	}
    
	obstacle.vel = vec2(r_float_range(0, 0.01f), r_float_range(0, 0.01f));
	obstacle.color = vec4(0,0,0,0);
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

	dynarr_push(&game->entities, &obstacle);
    return obstacle;
}

void generate_white_in_tunnel(game_t* game) {
	FOR (w, 30) {
		// entity_t obstacle = create_obstacle(
		// 	vec2(r_float_range(-3.0f, 3.0f),
		// 		r_float_range(WALL_TUNNEL_START, WALL_FINISH_START)-SCREEN_TOP),
		// 	r_float_range(1.0f, 2.0f),
		// 	vec4(1, 1, 1, 1), ASTEROID_WHITE);
		// 	dynarr_push(&game->entities, &obstacle);
		entity_t obstacle = create_obstacle(game, ASTEROID_WHITE, r_float_range(1.0f, 2.0f));
		dynarr_push(&game->entities, &obstacle);
	}
}

int set_obstacle_type(float red, float green, float blue, float yellow) {
    float r = r_float();
    int type = ASTEROID_WHITE;
    if (r < red) {
        type = ASTEROID_RED;
    } else if (r < green) {
        type = ASTEROID_GREEN;
    } else if (r < blue) {
        type = ASTEROID_BLUE;
    } else if (r < yellow) {
        type = ASTEROID_YELLOW;
    }

    return type;
}

float set_obstacle_size(int type, float redMax, float greenMax, float blueMax, float yellowMax, float whiteMax) {
    float size = 0;
    if (type == ASTEROID_RED) {
        size = r_float_range(1.0f, redMax);
    } else if (type == ASTEROID_GREEN) {
        size = r_float_range(1.0f, greenMax);
    } else if (type == ASTEROID_BLUE) {
        size = r_float_range(1.0f, blueMax);
    } else if (type == ASTEROID_YELLOW) {
        size = r_float_range(1.0f, yellowMax);
    } else {
        size = r_float_range(1.0f, whiteMax);
    }

    return size;
} 

void level1(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 1.0f;
    float redQT = 0.00f;

    float greenMax = 2.0f;
    float greenQT = 0.0f;

    float blueMax = 2.5f;
    float blueQT = 0.0f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 10.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 100) {
        int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level2(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 4.0f;
    float redQT = 0.1f;

    float greenMax = 3.0f;
    float greenQT = 0.2f;

    float blueMax = 2.5f;
    float blueQT = 0.0f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 150) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level3(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 6.0f;
    float redQT = 0.1f;

    float greenMax = 6.0f;
    float greenQT = 0.2f;

    float blueMax = 2.5f;
    float blueQT = 0.0f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 130) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level4(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 3.0f;
    float redQT = 0.4f;

    float greenMax = 6.0f;
    float greenQT = 0.5f;

    float blueMax = 2.5f;
    float blueQT = 0.0f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 130) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level5(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 3.0f;
    float redQT = 0.1f;

    float greenMax = 6.0f;
    float greenQT = 0.0f;

    float blueMax = 4.5f;
    float blueQT = 0.2f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 130) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level6(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 3.0f;
    float redQT = 0.1f;

    float greenMax = 6.0f;
    float greenQT = 0.2f;

    float blueMax = 4.5f;
    float blueQT = 0.3f;

    float yellowMax = 1.0f;
    float yellowQT = 0.0f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 200) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}


void level7(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 3.0f;
    float redQT = 0.1f;

    float greenMax = 6.0f;
    float greenQT = 0.0f;

    float blueMax = 4.5f;
    float blueQT = 0.0f;

    float yellowMax = 2.0f;
    float yellowQT = 0.2f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 130) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level8(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 3.0f;
    float redQT = 0.15f;

    float greenMax = 6.0f;
    float greenQT = 0.0f;

    float blueMax = 4.5f;
    float blueQT = 0.0f;

    float yellowMax = 2.0f;
    float yellowQT = 0.5f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 300) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level9(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 4.0f;
    float redQT = 0.15f;

    float greenMax = 4.0f;
    float greenQT = 0.25f;

    float blueMax = 5.5f;
    float blueQT = 0.40f;

    float yellowMax = 4.0f;
    float yellowQT = 0.5f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 270) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

void level10(game_t* game) {
	entity_t obstacle;
	int added = 0;

    float redMax = 4.0f;
    float redQT = 0.15f;

    float greenMax = 4.0f;
    float greenQT = 0.35f;

    float blueMax = 5.5f;
    float blueQT = 0.60f;

    float yellowMax = 4.0f;
    float yellowQT = 0.8f;

    float whiteMax = 4.0f;
    
	generate_white_in_tunnel(game);

	FOR (i, 250) {
	    int type = set_obstacle_type(redQT, greenQT, blueQT, yellowQT);
		create_obstacle(
            game, 
            type, 
            set_obstacle_size(type, redMax, greenMax, blueMax, yellowMax, whiteMax)
        );
	}
}

level_proc_t levels[] = {
    level1,
	level2,
	level3,
    level4,
    level5,
    level6,
    level7,
    level8,
    level9,
    level10,
};