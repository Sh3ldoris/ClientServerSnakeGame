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
    int ** field_server;
    int ** field_client;
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
    pthread_mutex_t* mut;
    pthread_cond_t* is_game_on;
    pthread_cond_t* can_draw;
    pthread_cond_t* can_read;
} DATA;


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
pthread_cond_t cond3;

pthread_t server_player;
pthread_t server;
pthread_t client_com;


void draw_arena();
int key_hit();
void connect_client(char *argv[]);
void draw_game();
void snake_init();
void* handle_server_player(void* arg);
void * handle_game(void* arg);
void* client_communication(void* args);
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
void opponent_left_screen();
void you_left_screen();
void wait_opponent_join_screen();
void wait_opponent_to_start_game_screen();
int send_message(char *message);

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);
    pthread_cond_init(&cond3, NULL);

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

    int ** server_field;
    int ** client_field;

    server_field = malloc(M * sizeof(int *));
    for (int i = 0; i < M; ++i)
        server_field[i] = malloc(N * sizeof(int));

    client_field = malloc(M * sizeof(int *));
    for (int i = 0; i < M; ++i)
        client_field[i] = malloc(N * sizeof(int));

    /// Create threads
    DATA data = {
            server_field,
            client_field,
            5,
            N-11,
            0,
            0,
            0,
            0,
            0,
            0,
            2,
            4,
            3,
            2,
            &mut,
            &cond1,
            &cond2,
            &cond3
    };

    pthread_create(&client_com, NULL, &client_communication, &data);
    pthread_create(&server_player, NULL, &handle_server_player, &data);
    pthread_create(&server, NULL, &handle_game, &data);

    pthread_join(client_com, NULL);
    pthread_join(server_player, NULL);
    pthread_join(server, NULL);


    pthread_cond_destroy(&cond1);
    pthread_mutex_destroy(&mut);

    for (int i = 0; i < N; ++i)
        free(data.field_server[i]);
    free(data.field_server);

    for (int i = 0; i < N; ++i)
        free(data.field_client[i]);
    free(data.field_client);
    close(newsockt);
    close(sockt);
    system("clear");
    endwin();
    printf("KKKKKKONIEC\n");
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

void* client_communication(void* args) {
    DATA * data = (DATA* ) args;

    int direction_change_client = 4;
    int drawn_already = 0;

    int len;
    char buff[256];
    int field1server[M][N];
    int field2client[M][N];
    bzero(buff,256);

    while (1) {
        len = read(newsockt, buff, 255);
        if (strcmp(buff, "play") == 0) {
            pthread_cond_broadcast(data->is_game_on);
            break;
        }
    }

    int i = 0;
    int game_stat = 3;
    while (game_stat == 3) {

        pthread_mutex_lock(data->mut);

        if (data->is_drawn == 2 || drawn_already == 1) {
            drawn_already = 0;
            pthread_cond_wait(data->can_draw, data->mut);
        }

        for (int j = 0; j < M; ++j) {
            for (int k = 0; k < N; ++k) {
                field1server[j][k] = data->field_server[j][k];
                field2client[j][k] = data->field_client[j][k];
            }

        }

        /// Pocuvame klienta na zmeny pohybu

        len = read(newsockt, &direction_change_client, sizeof(direction_change_client));
        if (len < 0) {
            perror("Error writing to socket");
            break;
        }
        mvprintw(M+3, 0,"ND: %d  s krokom: %d", direction_change_client, ++i);
        data->direction_change_client = direction_change_client;

        usleep(1000);
        n = write(newsockt, field1server, sizeof(field1server));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, field2client, sizeof(field2client));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->head_server, sizeof(data->head_server));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->head_client, sizeof(data->head_client));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->score_server, sizeof(data->score_server));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->score_client, sizeof(data->score_client));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->fruit_x, sizeof(data->fruit_x));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->fruit_y, sizeof(data->fruit_y));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->fruit_value, sizeof(data->fruit_value));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->is_fruit_generated, sizeof(data->is_fruit_generated));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);
        n = write(newsockt, &data->game_status, sizeof(data->game_status));
        if (n < 0)
        {
            perror("Error writing to socket");
            break;
        }

        data->is_drawn++;
        drawn_already = 1;
        pthread_mutex_unlock(data->mut);

        if (data->is_drawn == 2) {
            pthread_cond_signal(data->can_read);
        }
    }

    mvprintw(M+8, 0, "Ahooooooj");
    if ((data->game_status != 1) || (data->game_status != 2)) {
        pthread_mutex_lock(data->mut);
        data->game_status = 0;
        pthread_mutex_unlock(data->mut);
    }
    sleep(10);

}

void* handle_server_player(void* arg) {
    DATA* data = (DATA*) arg;

    usleep(10);
    pthread_mutex_lock(data->mut);
    if (data->game_status == 3) {
        pthread_cond_wait(data->is_game_on, data->mut);
    }
    pthread_mutex_unlock(data->mut);
    int c = 0;
    int direction_change = 2;
    int drawn_already = 0;

    draw_arena();
    /// Countdown from 3
    //countdown();
    int game_stat = 3;
    int ch = 0;
    int i = 0;
    while(game_stat == 3) {
        ch = getch();
        pthread_mutex_lock(data->mut);

        if (data->is_drawn == 2 || drawn_already == 1) {
            drawn_already = 0;
            pthread_cond_wait(data->can_draw, data->mut);
        }
        /// Get input
        if (ch != ERR) {
            c = ch;
            if (c == 97)
                direction_change = 4;
            if (c == 100)
                direction_change = 2;
            if (c == 120)
                game_stat = 0;
            if (c == 119)
                direction_change = 1;
            if (c == 115)
                direction_change = 3;
        }
        data->direction_change_server = direction_change;

        mvprintw(M + 5, 0, "Head Klient: %d", data->head_client);
        mvprintw(M + 6, 0, "Head Server: %d", data->head_server);

        for(int i = 1; i <= M - 1; i++){
            for (int j = 1; j <= N - 1; j++) {
                if (((data->field_server[i][j] > 0) && (data->field_server[i][j] < data->head_server)) || ((data->field_client[i][j] > 0) && (data->field_client[i][j] < data->head_client))) {
                    mvprintw(i, j, "o");
                } else if ((data->field_server[i][j] == data->head_server) || (data->field_client[i][j] == data->head_client)) {
                    mvprintw(i, j, "x");
                } else if (data->is_fruit_generated == 1 && j == data->fruit_x && i == data->fruit_y) {
                    mvprintw(i, j, "%d",data->fruit_value);
                } else {
                    mvprintw(i, j, " ");
                }
                /// !!! Bacha na ELSE vetvu !!!
            }
        }

        move(M + 10, 0);
        for(int i = 1; i <= M - 1; i++){
            for (int j = 1; j <= N - 1; j++) {
                printw("%d", data->field_server[i][j]);
            }
            printw("\n");
        }
        printw("\n");

        mvprintw(M + 2, (N/2) - 17, "Your Score: %d  Opponent's Score: %d", data->score_server, data->score_client);
        move(M + 3, 0);
        game_stat = data->game_status;
        data->is_drawn++;
        drawn_already = 1;
        pthread_mutex_unlock(data->mut);

        if (data->is_drawn == 2) {
            pthread_cond_signal(data->can_read);
        }

        refresh();
    }
    refresh();

    draw_game_over();

    sleep(2);

    switch (game_stat) {
        case 1:
            winner_screen();
            break;
        case 2:
            loser_screen();
            break;
        default:
            opponent_left_screen();
            break;
    }

}

void * handle_game(void* arg) {
    DATA* data = (DATA*) arg;

    pthread_mutex_lock(data->mut);
    if (data->game_status == 3) {
        pthread_cond_wait(data->is_game_on, data->mut);
        mvprintw(M + 7, 0 , "Som v handle game, som za podmienkou");
        refresh();
    }
    pthread_mutex_unlock(data->mut);

    int field1[M][N] = {0}; /// Pozicia pre hraca 1
    int field2[M][N] = {0}; /// Pozicia pre hraca 2
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

    int play = 3;

    int direction_change_server;
    int direction_change_client = 4;
    int direction_server = 2;
    int direction_client = 4;

    int j1 = x1;
    int j2 = x2;
    for (int i = 1; i <= head1; ++i) {
        field1[y_1][++j1 - head1] = i;
        field2[y2][j2++] = head2 - (i - 1);
    }



    while (play == 3) {
        pthread_mutex_lock(data->mut);


        if (data->is_drawn  < 2) {
            pthread_cond_wait(data->can_read, data->mut);
        }


        //usleep(200000);
        //sleep(1);
       /* /// Pocuvame klienta na zmeny pohybu
        len = read(newsockt, &direction_change_client, sizeof(direction_change_client));
        if (len < 0) {
            break;
        }*/
        ///Zmena hry
        direction_change_server = data->direction_change_server;
        direction_change_client = data->direction_change_client;

        if (fruit_generated == 0) {
            fruit_x = (rand() % (N - 2) ) + 1;
            fruit_y = (rand() % (M - 2) ) + 1;

            fruit_value = (rand() % 3) + 1;
            /// Pokial sa vygeneruje napr v tele hada tak sa nezobrazi
            if ((field1[fruit_y][fruit_x] == 0) || (field2[fruit_y][fruit_x] == 0))
                fruit_generated = 1;
        }

        switch (direction_change_server) {
            case 1:
                if (direction_server == 3)
                    break;
                direction_server = 1;
                break;
            case 2:
                if (direction_server == 4)
                    break;
                direction_server = 2;
                break;
            case 3:
                if (direction_server == 1)
                    break;
                direction_server = 3;
                break;
            case 4:
                if (direction_server == 2)
                    break;
                direction_server = 4;
                break;
            default:
                break;
        }

        switch (direction_server) {
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

        switch (direction_change_client) {
            case 1:
                if (direction_client == 3)
                    break;
                direction_client = 1;
                break;
            case 2:
                if (direction_client == 4)
                    break;
                direction_client = 2;
                break;
            case 3:
                if (direction_client == 1)
                    break;
                direction_client = 3;
                break;
            case 4:
                if (direction_client == 2)
                    break;
                direction_client = 4;
                break;
            default:
                play = 0;
                break;
        }

        switch (direction_client) {
            case 1:
                if (y2-- <= 1)
                    y2 = M - 1;
                break;
            case 2:
                if (x2++ >= N - 1)
                    x2 = 1;
                break;
            case 3:
                if (y2++ >= M - 1)
                    y2 = 1;
                break;
            case 4:
                if (x2-- <= 1)
                    x2 = N - 1;
                break;
            default:
                break;
        }

        /// Fruit ham check
        if (fruit_x == x1 && fruit_y == y_1) {
            fruit_generated = 0;
            for (int i = 0; i < fruit_value; ++i) {
                tail1--;
                current_score1++;
            }
        }
        if (fruit_x == x2 && fruit_y == y2) {
            fruit_generated = 0;
            for (int i = 0; i < fruit_value; ++i) {
                tail2--;
                current_score2++;
            }
        }

        /// Collision check
        if ((field1[y_1][x1] != 0) || (field2[y_1][x1] != 0)) {
                play = 2;
                data->game_status = 2;
        }

        if ((field2[y2][x2] != 0) ||  (field1[y2][x2] != 0)) {
            play = 1;
            data->game_status = 1;
        }

        //// Step
        field1[y_1][x1] = ++head1;
        field2[y2][x2] = ++head2;

        /// shift tail
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N; ++j) {
                if (field1[i][j] == tail1)
                    field1[i][j] = 0;
                if (field2[i][j] == tail2)
                    field2[i][j] = 0;
            }
        }
        tail1++;
        tail2++;

        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N; ++j) {
                data->field_server[i][j] = field1[i][j];
                data->field_client[i][j] = field2[i][j];
            }
        }

        data->head_server = head1;
        data->head_client = head2;

        data->score_server = current_score1;
        data->score_client = current_score2;

        data->fruit_x = fruit_x;
        data->fruit_y = fruit_y;
        data->fruit_value = fruit_value;
        data->is_fruit_generated = fruit_generated;

        data->game_status = play;

        data->is_drawn = 0;
        pthread_cond_broadcast(data->can_draw);
        pthread_mutex_unlock(data->mut);

        /// Struct medzi server a serverHracom field 1,2 score 1,2, fruit x,y
        /// Odosielame zmeny
        /*if (fruit_generated == 0) {
            generate_fruit();
        }*/

        usleep(200000);
    }

    return NULL;
}

/**
 * Initialization of snake
 */
/*void snake_init() {
    int j1 = x1;
    int j2 = x2;
    for (int i = 1; i <= head1; ++i) {
        field1[y_1][++j1 - head1] = i;
        field2[y2][j2++] = head2 - (i - 1);
    }
}*/

/*void draw_game() {
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
}*/

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
    //draw_game();
    for (int i = 3; i > 0; --i) {
        attr_on(COLOR_PAIR(3),0);
        mvprintw(M / 2, (N/2), "%d", i);
        move(M + 1,0);
        attr_off(COLOR_PAIR(3),0);
        refresh();
        sleep(1);
    }
}

/*void generate_fruit() {
    fruit_x = (rand() % (N - 2) ) + 1;
    fruit_y = (rand() % (M - 2) ) + 1;

    fruit_value = (rand() % 3) + 1;
    /// Pokial sa vygeneruje napr v tele hada tak sa nezobrazi
    if (field1[fruit_y][fruit_x] == 0)
        fruit_generated = 1;
}*/

/**
 * Checks if snake is at fruit position
 */
/*void eat_fruit(){
    if ((fruit_x) == x1 && fruit_y == y_1) {
        fruit_generated = 0;
        for (int i = 0; i < fruit_value; ++i) {
            tail1--;
            current_score1++;
        }
    }
}*/

/**
 * Checks snake collision with itself
 */
/*void check_collision() {
    if (field1[y_1][x1] != 0)
        play = 0;
}*/

/**
 * Change direction of movement
 * @param change
 */
/*void step(int change) {
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
}*/

/*void share_info() {
    char info[256];
    bzero(info, strlen(info));
    sprintf(info, "%d;%d;%d;%d;%d;%d;%d;%d;%d", x1, y_1, head1, tail1, current_score1, fruit_x, fruit_y,fruit_value, game_status);
    //mvprintw(M + 4, 0, info);
    send_message(info);
    usleep(1000);
}*/

int send_message(char *message) {
    n = write(newsockt, message, strlen(message));
    if (n < 0)
        return -1;
    return 0;
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
    //mvprintw(M/2 + 5,  N/2 - 11, "	 Your SCORE: %d !", current_score1);

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
    //mvprintw(M/2 + 5, N/2 - 10, "  Your SCORE: %d !", current_score1);

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n') {
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);
}

void opponent_left_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3),0);
    mvprintw(M/2 - 7, N/2 - 10,"         / \\   ");
    mvprintw(M/2 - 6,  N/2 - 10,"        /   \\  ");
    mvprintw(M/2 - 5,  N/2 - 10,"       /     \\");
    mvprintw(M/2 - 4,  N/2 - 10,"      /   _   \\ ");
    mvprintw(M/2 - 3,  N/2 - 10,"     /   | |   \\ ");
    mvprintw(M/2 - 2,  N/2 - 10,"    /    | |    \\ ");
    mvprintw(M/2 - 1,  N/2 - 10,"   /     |_|     \\ ");
    mvprintw(M/2 ,  N/2 - 10,   "  /               \\ ");
    mvprintw(M/2 + 1,  N/2 - 10," /        O        \\ ");
    mvprintw(M/2 + 2,  N/2 - 10,"/___________________\\ ");
    attr_off(COLOR_PAIR(3),0);

    mvprintw(M/2 + 4,  N/2 - 13,"Ooops! Something went WRONG!");
    mvprintw(M/2 + 5,  N/2 - 5, "We 're SORRY!");

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n'){
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);
}

void you_left_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3),0);
    mvprintw(M/2 - 7, N/2 - 10,"    /-----------\\     ");
    mvprintw(M/2 - 6,  N/2 - 10,"   /             \\  ");
    mvprintw(M/2 - 5,  N/2 - 10,"  /               \\  ");
    mvprintw(M/2 - 4,  N/2 - 10," |     O     O     |  ");
    mvprintw(M/2 - 3,  N/2 - 10," |                 | ");
    mvprintw(M/2 - 2,  N/2 - 10," |     _______     |");
    mvprintw(M/2 - 1,  N/2 - 10,"  \\   /       \\   / ");
    mvprintw(M/2 ,  N/2 - 10,   "   \\             / ");
    mvprintw(M/2 + 1,  N/2 - 10,"    \\___________/");
    attr_off(COLOR_PAIR(3),0);

    mvprintw(M/2 + 4,  N/2 - 9,"You LEFT the GAME!");
    mvprintw(M/2 + 5,  N/2 - 12, "We HOPE you'll come BACK!");

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n'){
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);
}