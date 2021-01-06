#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

/// Rozmery plochy
#define N  50
#define M  18

typedef struct game_data {
    int socket_descriptor;
    int play;
    int writeIndex;
    int readIndex;
    char ** server_buffer;
    pthread_mutex_t* mut;
    pthread_cond_t* is_game_on;
    pthread_cond_t* can_read;
} DATA;

int field1[M][N] = {0}; /// Pozicia pre hraca 1
int field2[M][N] = {0}; /// Pozicia pre hraca 2
int direction = 2; /// Smer pohybu (1-4 clockwise)
int head1 = 5;
int tail1 = 1;
int y_1 = M / 4;
int x1 = 10;
int current_score1 = 0;

int head2 = 5;
int tail2 = 1;
int y2 = (M / 4) * 3 + 1;
int x2 = N - 10;
int current_score2 = 0;

int fruit_generated = 0;
int fruit_x = 10;
int fruit_y = 7;
int fruit_value = 0;

int play = 0;

/**
 * 0 - nehra sa
 * 1 - vitaz je hrac jedna (server)
 * 2 - vitaz je hrac dva (klient)
 * 3 - hra sa
 */
int game_status = 0;

int sockt, newsockt;
socklen_t cli_len;
struct sockaddr_in serv_addr, cli_addr;
int n;
char buffer[256];

pthread_mutex_t mut;
pthread_cond_t cond1;
pthread_cond_t cond2;

pthread_t server_player;
pthread_t client;


void draw_arena();
int key_hit();
void connect_client(char *argv[]);
void draw_game();
void snake_init();
void* play_game(void* arg);
void * listen_client(void* arg);
void draw_arena();
void draw_game_over();
void countdown();
void generate_fruit();
void eat_fruit();
void check_collision();
void step(int change);
void share_info();
void start_screen();
void loser_screen();
void winner_screen();
void wait_opponent_join_screen();
void wait_opponent_to_start_game_screen();
int send_message(char *message);

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    /// color pairs used for color graphics
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);

    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr,"usage %s port\n", argv[0]);
        return 1;
    } else {
        start_screen();
        connect_client(argv);
    }

    char* array[20];
    /// Create threads
    DATA data = {newsockt, 1, 0, 0, array, &mut, &cond1, &cond2};
    mvprintw(M+4, 0, "Inicializovanie");
    pthread_create(&server_player, NULL, &play_game, &data);
    pthread_create(&client, NULL, &listen_client, &data);

    pthread_join(server_player, NULL);
    pthread_join(client, NULL);


    pthread_cond_destroy(&cond1);
    pthread_mutex_destroy(&mut);

    close(newsockt);
    close(sockt);
    endwin();
    return 0;
}


int key_hit() {
    int ch = getch();

    if (ch != ERR) {
        ungetch(ch);
        return 1;
    } else {
        return 0;
    }
}

void connect_client(char *argv[]) {
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockt = socket(AF_INET, SOCK_STREAM, 0);
    if (sockt < 0) {
        perror("Error creating socket");
        endwin();
        exit(1);
    }

    if (bind(sockt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket address");
        endwin();
        exit(2);
    }

    listen(sockt, 1);
    cli_len = sizeof(cli_addr);

    wait_opponent_join_screen();
    newsockt = accept(sockt, (struct sockaddr*)&cli_addr, &cli_len);
    if (newsockt < 0) {
        perror("Protivnikovi sa nepodarilo pripojit!");
        endwin();
        exit(3);
    }
    wait_opponent_to_start_game_screen();
}

void* play_game(void* arg) {
    DATA* data = (DATA*) arg;

    pthread_mutex_lock(data->mut);

    if (data->play == 1) {
        mvprintw(M+4, 0, "Cakanie");
        pthread_cond_wait(data->is_game_on, data->mut);
    }
    pthread_mutex_unlock(data->mut);

    int c = 0;
    int direction_change = 2;

    snake_init();
    draw_arena();

    /// Countdown from 3
    countdown();

    pthread_mutex_lock(data->mut);
    play = data->play;
    pthread_mutex_unlock(data->mut);


    while(play) {
        /// 1. Odosli udaje
        /// 2. Nacitaj udaje zo struktury
        /// Pokial bude struktura prazdna (readI == writeI) => cakaj na data.can_read


        pthread_mutex_lock(data->mut);
        play = data->play;
        pthread_mutex_unlock(data->mut);

        /// Generate fruit
        if (fruit_generated == 0) {
            generate_fruit();
        }

        /// Print play_game area
        draw_game();


        /// Get input
        if (key_hit()) {
            c = getch();
            if (c == 97)
                direction_change = 4;
            if (c == 100)
                direction_change = 2;
            if (c == 120)
                play = 0;
            if (c == 119)
                direction_change = 1;
            if (c == 115)
                direction_change = 3;
        }

        step(direction_change);

        usleep(200000);
    }
    refresh();

    draw_game_over();

    sleep(2);

    if (current_score1 == 0)
        loser_screen();
    if (current_score1 > 0)
        winner_screen();

}

void * listen_client(void* arg) {
    int len;
    DATA* data = (DATA*) arg;
    char buffer[256];
    bzero(buffer,strlen(buffer));
    while (data->play && (len = read(newsockt, buffer, strlen(buffer)))) {
        if (strcmp(buffer, "play") == 0) {
            mvprintw(M+ 7, 0, "Stlacil play");
            pthread_cond_signal(data->is_game_on);
        } /*else if  (strcmp(buffer, "getinfo")) {
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "toto su nove udaje zo servera");
            n = write(newsockt, buffer, strlen(buffer));
            if  (n < 0)
                exit(6);
            mvprintw(M + 10, 0, "GET request koniec  ");
        }*/
    }
}

/**
 * Initialization of snake
 */
void snake_init() {
    int j1 = x1;
    int j2 = x2;
    for (int i = 1; i <= head1; ++i) {
        field1[y_1][++j1 - head1] = i;
        field2[y2][j2++] = head2 - (i - 1);
    }
}

void draw_game() {
    for(int i = 1; i <= M - 1; i++){
        for (int j = 1; j <= N - 1; j++) {
             if (((field1[i][j] >= tail1) && (field1[i][j] < head1)) || ((field2[i][j] >= tail2) && (field2[i][j] < head2))) {
                mvprintw(i, j, "o");
            } else if ((field1[i][j] == head1) || (field2[i][j] == head2)) {
                 mvprintw(i, j, "x");
            } else if (fruit_generated == 1 && j == fruit_x && i == fruit_y) {
                 mvprintw(i, j, "%d",fruit_value);
            } else {
                 mvprintw(i, j, " ");
            }
            /// !!! Bacha na ELSE vetvu !!!
        }
    }
    mvprintw(M + 2, (N/2) - 17, "Your Score: %d  Opponent's Score: %d", current_score1, current_score2);
    move(M + 3, 0);
    refresh();
}

void draw_arena() {
    move(0,0);
    attr_on(COLOR_PAIR(2),0);
    for(int i=0;i<=M;i++){
        for (int j = 0; j <= N; ++j) {
            if ((i == 0 && j == 0) || (i == 0 && j == N) || (i == M && j == 0) || (i == M && j == N)) {
                printw("+");
            }
            if ((i == 0 && j > 0 && j < N) || (i == M && j > 0 && j < N)) {
                printw("-");
            }
            if ((i > 0 && i < M && j == 0) || (i > 0 && i < M && j == N)) {
                printw("|");
            }
            if (i > 0 && i < M && j > 0 && j < N) {
                printw(" ");
            }
        }
        printw("\n");
    }
    attr_off(COLOR_PAIR(2),0);
    refresh();
}

void draw_game_over() {
    attr_on(COLOR_PAIR(3),0);
    mvprintw((M / 2) - 1, (N/2) - 6, "           ");
    mvprintw(M / 2, (N/2) - 6, " Game OVER ");
    mvprintw((M / 2) + 1, (N/2) - 6, "           ");
    move(0,0);
    attr_off(COLOR_PAIR(3),0);
    move(M + 1, 0);
    refresh();
}

void countdown() {
    draw_game();
    for (int i = 3; i > 0; --i) {
        attr_on(COLOR_PAIR(3),0);
        mvprintw(M / 2, (N/2), "%d", i);
        move(M + 1,0);
        attr_off(COLOR_PAIR(3),0);
        refresh();
        sleep(1);
    }
}

void generate_fruit() {
    fruit_x = (rand() % (N - 2) ) + 1;
    fruit_y = (rand() % (M - 2) ) + 1;

    fruit_value = (rand() % 3) + 1;
    /// Pokial sa vygeneruje napr v tele hada tak sa nezobrazi
    if (field1[fruit_y][fruit_x] == 0)
        fruit_generated = 1;
}

/**
 * Checks if snake is at fruit position
 */
void eat_fruit(){
    if ((fruit_x) == x1 && fruit_y == y_1) {
        fruit_generated = 0;
        for (int i = 0; i < fruit_value; ++i) {
            tail1--;
            current_score1++;
        }
    }
}

/**
 * Checks snake collision with itself
 */
void check_collision() {
    if (field1[y_1][x1] != 0)
        play = 0;
}

/**
 * Change direction of movement
 * @param change
 */
void step(int change) {
    switch (change) {
        case 1:
            if (direction == 3)
                break;
            direction = 1;
            break;
        case 2:
            if (direction == 4)
                break;
            direction = 2;
            break;
        case 3:
            if (direction == 1)
                break;
            direction = 3;
            break;
        case 4:
            if (direction == 2)
                break;
            direction = 4;
            break;
        default:
            break;
    }

    switch (direction) {
        case 1:
            if (y_1-- <= 1)
                y_1 = M - 1;
            break;
        case 2:
            if (x1++ >= N - 1)
                x1 = 1;
            break;
        case 3:
            if (y_1++ >= M - 1)
                y_1 = 1;
            break;
        case 4:
            if (x1-- <= 1)
                x1 = N - 1;
            break;
        default:
            break;
    }

    eat_fruit();
    check_collision();

    field1[y_1][x1] = ++head1;

    /// shift tail1
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field1[i][j] == tail1)
                field1[i][j] = 0;
        }
    }
    tail1++;
}

void share_info() {
    char info[256];
    bzero(info, strlen(info));
    sprintf(info, "%d;%d;%d;%d;%d;%d;%d;%d", x1, y_1, head1, tail1, current_score1, fruit_x, fruit_y, game_status);
    //mvprintw(M + 4, 0, info);
    send_message(info);
    usleep(1000);
}

void start_screen() {
    system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(1),0);
    mvprintw(M/2 - 4, N/2 - 20, "    ________         ________  ");
    mvprintw(M/2 - 3,  N/2 - 20, "   /        \\       /        \\        0 ");
    mvprintw(M/2 - 2,  N/2 - 20 , "  /  /----\\  \\     /  /----\\  \\       0  ");
    mvprintw(M/2 - 1,  N/2 - 20 , "  |  |    |  |     |  |    |  |      / \\ ");
    mvprintw(M/2 ,  N/2 - 20, "  |  |    |  |     |  |    |  |     /  |  ");
    mvprintw(M/2 + 1,  N/2 - 20," (o  o)   \\   \\___/   /    \\   \\___/   / ");
    mvprintw(M/2 + 2,  N/2 - 20 , "  \\__/     \\         /      \\         /  ");
    mvprintw(M/2 + 3,  N/2 - 20 , "            --------         -------- 	");
    attr_off(COLOR_PAIR(1),0);

    mvprintw(M/2 + 5,  N/2 - 12 ,"Start when you are ready!");
    refresh();

    attr_on(COLOR_PAIR(4),0);
    while (getch() != '\n')  /// caka na enter
    {
        mvprintw(M/2 + 6,  N/2 - 10 , "Press ENTER to START.");
        move(M + 1, 0);
        refresh();
        sleep(1);
        mvprintw(M/2 + 6,  N/2 - 10 ,"                     ");
        move(M + 1, 0);
        refresh();
        sleep(1);

    }
    attr_off(COLOR_PAIR(4),0);
}

void wait_opponent_join_screen() {
    draw_arena();

    attr_on(COLOR_PAIR(1),0);
    mvprintw(M/2 - 4, N/2 - 10, "     /");
    mvprintw(M/2 - 3,  N/2 - 10, "   \\/ ");
    mvprintw(M/2 - 2,  N/2 - 18 , "  .... ->->->->->");
    mvprintw(M/2 - 1,  N/2 - 18 , "  |  |            ");
    mvprintw(M/2 ,  N/2 - 18, "  |  |                ");
    mvprintw(M/2 + 1,  N/2 - 18," (o  o)             ");
    mvprintw(M/2 + 2,  N/2 - 18 , "  \\__/           ");
    mvprintw(M/2 + 3,  N/2 - 18 , "      	");
    attr_off(COLOR_PAIR(1),0);
    attr_on(COLOR_PAIR(3),0);
    mvprintw(M/2 - 4, N/2 + 5, " \\/");
    mvprintw(M/2 - 3,  N/2 + 5, " /\\ ");
    mvprintw(M/2 - 2,  N/2 + 2 , "<-<-<-<-<- .... ");
    mvprintw(M/2 - 1,  N/2 + 2 , "           |  |");
    mvprintw(M/2 ,  N/2 + 2, "           |  |   ");
    mvprintw(M/2 + 1,  N/2 + 2,"          (o  o)   ");
    mvprintw(M/2 + 2,  N/2 + 2 , "           \\__/   ");
    mvprintw(M/2 + 3,  N/2 + 2, "      	");
    attr_off(COLOR_PAIR(3),0);

    attr_on(COLOR_PAIR(4), 0);
    mvprintw(M / 2 + 6, N / 2 - 15, "Waiting for OPPONENT to join...");
    move(M + 2, 0);
    refresh();
    attr_off(COLOR_PAIR(4),0);
}

void wait_opponent_to_start_game_screen() {
    //draw_arena();
    attr_on(COLOR_PAIR(1),0);
    mvprintw(M/2 - 4, N/2 - 10, "     /            /");
    mvprintw(M/2 - 3,  N/2 - 10, "   \\/          \\/ ");
    mvprintw(M/2 - 2,  N/2 - 18 , "  .... ----------------------- .... ");
    mvprintw(M/2 - 1,  N/2 - 18 , "  |  |                         |  |");
    mvprintw(M/2 ,  N/2 - 18, "  |  |                         |  |   ");
    mvprintw(M/2 + 1,  N/2 - 18," (o  o)                       (o  o)   ");
    mvprintw(M/2 + 2,  N/2 - 18 , "  \\__/                         \\__/   ");
    attr_off(COLOR_PAIR(1),0);
    mvprintw(M/2 + 4,  N/2 - 15, " Opponent JOINED SUCCESSFULLY.");

    attr_on(COLOR_PAIR(4), 0);
    mvprintw(M / 2 + 6, N / 2 - 18, "Waiting for OPPONENT to START GAME...");
    move(M + 2, 0);
    refresh();
    usleep(100000);

    attr_off(COLOR_PAIR(4),0);
}

void loser_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3),0);
    mvprintw(M/2 - 7, N/2 - 11,"         ________   ");
    mvprintw(M/2 - 6,  N/2 - 11,"        /        \\  ");
    mvprintw(M/2 - 5,  N/2 - 11,"       /  /----\\  \\  ");
    mvprintw(M/2 - 4,  N/2 - 11,"       |  |    |  |  ");
    mvprintw(M/2 - 3,  N/2 - 11,"       |  |    |  | ");
    mvprintw(M/2 - 2,  N/2 - 11,"      (o  o)   |  | ");
    mvprintw(M/2 - 1,  N/2 - 11,"  |\\   \\__/    |  | ");
    mvprintw(M/2 ,  N/2 - 11,"  | \\__________/  / ");
    mvprintw(M/2 + 1,  N/2 - 11,"  \\              / ");
    mvprintw(M/2 + 2,  N/2 - 11,"   -------------  	");
    attr_off(COLOR_PAIR(3),0);

    mvprintw(M/2 + 4,  N/2 - 11,"	   Game OVER!");
    mvprintw(M/2 + 5,  N/2 - 11, "	 Your SCORE: %d !", current_score1);

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n'){
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);
}

void winner_screen() {
    draw_arena();
    attr_on(COLOR_PAIR(4),0);
    mvprintw(M/2 - 7, N/2 - 22,"      _________________________________  ");
    mvprintw(M/2 - 6, N/2 - 22,"     /                                 \\ ");
    mvprintw(M/2 - 5, N/2 - 22,"    /  /-----------------------------\\  \\ ");
    mvprintw(M/2 - 4, N/2 - 22,"    |  |          . . . . . . .      |  | ");
    mvprintw(M/2 - 3, N/2 - 22,"    |  |          |\\/\\/\\/\\/\\/\\|      |  | ");
    mvprintw(M/2 - 2, N/2 - 22,"   (o  o)         | o o o o o |      |  | ");
    mvprintw(M/2 - 1, N/2 - 22,"    \\__/    |\\    |___________|      |  | 	");
    mvprintw(M/2 , N/2 - 22,"            | \\______________________/  /  ");
    mvprintw(M/2 + 1, N/2 - 22,"            \\                          /  ");
    mvprintw(M/2 + 2, N/2 - 22,"             --------------------------  ");
    attr_off(COLOR_PAIR(4),0);

    mvprintw(M/2 + 4, N/2 - 10,"     You WON!");
    mvprintw(M/2 + 5, N/2 - 10, "  Your SCORE: %d !", current_score1);

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n') {
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);
}

int send_message(char *message) {
    n = write(newsockt, message, strlen(message));
    if (n < 0)
        return -1;
    return 0;
}
