
#ifndef SNAKEGAME_CLIENT_SERVER_H
#define SNAKEGAME_CLIENT_SERVER_H

/// Rozmery plochy
#define N  50
#define M  18

typedef struct game_data {
    int **field_server;
    int **field_client;
    int head_server;
    int head_client;
    int score_server;
    int score_client;
    int fruit_x;
    int fruit_y;
    int fruit_value;
    int is_fruit_generated;
    int direction_change_server;
    int direction_change_client;
    int game_status;
    int is_drawn;
    pthread_mutex_t *mut;
    pthread_cond_t *is_game_on;
    pthread_cond_t *can_draw;
    pthread_cond_t *can_read;
} DATA;

/**
 * GAME_STATUS
 * 0 - nehra sa
 * 1 - vitaz je hrac jedna (server)
 * 2 - vitaz je hrac dva (klient)
 * 3 - hra sa
 */

int sockt, newsockt;
socklen_t cli_len;
struct sockaddr_in serv_addr, cli_addr;


pthread_mutex_t mut;
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_cond_t cond3;

pthread_t server_player;
pthread_t server;
pthread_t client_com;


void draw_arena();

void connect_client(char *argv[]);

void *handle_server_player(void *arg);

void *handle_game(void *arg);

void *client_communication(void *args);

void draw_arena();

void draw_game_over();

void countdown();

void start_screen();

void loser_screen();

void winner_screen();

void opponent_left_screen();

void you_left_screen();

void wait_opponent_join_screen();

void wait_opponent_to_start_game_screen();

#endif //SNAKEGAME_CLIENT_SERVER_H
