
#ifndef SNAKEGAME_CLIENT_CLIENT_H
#define SNAKEGAME_CLIENT_CLIENT_H

/// Rozmery plochy
#define N  50
#define M  18

int field1[M][N] = {0}; /// Pozicia pre hraca 1
int field2[M][N] = {0}; /// Pozicia pre hraca 2

int head1 = 5;
int current_score1 = 0;

int head2 = 5;
int current_score2 = 0;

int fruit_generated = 1;
int fruit_x = 51;
int fruit_y = 1;
int fruit_value = 0;

int game_status = 3;

int sockt, n;
int sockfd;
int was_countdown;
struct sockaddr_in serv_addr;
struct hostent *server;


char buffer[256];

void force_end_handler(int signum);

void draw_arena();

int key_hit();

void draw_game();

void draw_arena();

void draw_game_over();

void start_screen();

void loser_screen();

void winner_screen();

void opponent_left_screen();

void you_left_screen();

void countdown();

#endif //SNAKEGAME_CLIENT_CLIENT_H
