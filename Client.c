#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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
    char** server_buffer;
    int count;
    pthread_mutex_t* mut;
    pthread_cond_t* is_game_on;
    pthread_cond_t* can_read;
    pthread_cond_t* can_write;
} DATA;

int field1[M][N] = {0}; /// Pozicia pre hraca 1
int field2[M][N] = {0}; /// Pozicia pre hraca 2
int direction = 4; /// Smer pohybu (1-4 clockwise)
int head1 = 5;
int tail1 = 1;
int y_1 = (M / 4) * 3 + 1;
int x1 = N - 10;
int current_score1 = 0;

int head2 = 5;
int tail2 = 1;
int y2 = M / 4;
int x2 = 10;
int current_score2 = 0;

int fruit_generated = 0;
int fruit_x = 10;
int fruit_y = 7;
int fruit_value = 0;

int play = 0;
int game_status = 0;

int sockt, n;
struct sockaddr_in serv_addr;
struct hostent* server;

char buffer[256];

pthread_mutex_t mut;
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_cond_t cond3;

pthread_t server_player;
pthread_t client;

void draw_arena();
int key_hit();
void connect_to_server(char *argv[]);
void* play_game(void* arg);
void * listen_server(void* arg);
void draw_game();
void snake_init();
void draw_arena();
void draw_game_over();
void countdown();
void eat_fruit();
void check_collision();
void step(int change);
void start_screen();
void loser_screen();
void winner_screen();
void set_values(char args[256]);
int send_message(char *message);

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);
    pthread_cond_init(&cond3, NULL);

    if (argc < 2) {
        fprintf(stderr,"usage %s port\n", argv[0]);
        return 1;
    } else {
        connect_to_server(argv);
    }

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

    start_screen();
    char * array[256];
    /// Create threads
    DATA data = {sockt, 1, 1, 1, array, 0, &mut, &cond1, &cond2, &cond3};

    pthread_create(&client, NULL, &play_game, &data);
    pthread_create(&server_player, NULL, &listen_server, &data);

    pthread_join(server_player, NULL);
    pthread_join(client, NULL);

    pthread_cond_destroy(&cond1);
    pthread_cond_destroy(&cond2);
    pthread_cond_destroy(&cond3);
    pthread_mutex_destroy(&mut);

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

void connect_to_server(char *argv[]) {

    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        exit(2);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockt = socket(AF_INET, SOCK_STREAM, 0);
    if (sockt < 0) {
        perror("Error creating socket");
        exit(3);
    }

    if(connect(sockt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to socket");
        exit(4);
    }

    usleep(100);
}

void* play_game(void* arg) {
    DATA* data = (DATA*) arg;

    int c = 0;
    int direction_change = direction;

    snake_init();
    draw_arena();

    /// Countdown from 3
    countdown();

    while(play) {

        /// 1. Posli udaje na server -- toto je skor nacitaj udaje z buffra of servra
        /// 2. Nastav udaje
        pthread_mutex_lock(data->mut);
        while (data->count <= 0) {
            pthread_cond_wait(data->can_read, data->mut);
            mvprintw(M + 8, 0, "READER CAKA                                                                      ");
        }

        if (data->count > 0){
            char buf[256];
            bzero(buf,256);
            strcpy(buf,data->server_buffer[data->readIndex]);

            mvprintw(M + 8, 0, "READER: %s     ", buf);
            mvprintw(M + 9, 0, "READER cita z indexu %d:                                                                              ",
                     data->readIndex);
            if (data->readIndex++ == 21)
                data->readIndex = 1;
            data->count--;
            pthread_cond_signal(data->can_write);
        }
/*        mvprintw(M + 4, 0, "Aktualne zapisny %d  ", data->writeIndex);
        if  (data->writeIndex == 2)
            mvprintw(M + 6, 0, "Citam na 1: %s       ", data->server_buffer[1]);*/
        pthread_mutex_unlock(data->mut);

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

        pthread_mutex_lock(data->mut);
        step(direction_change);
        data->play = play;
        pthread_mutex_unlock(data->mut);

        //usleep(30);
    }
    refresh();

    draw_game_over();

    sleep(2);

    if (current_score1 == 0)
        loser_screen();
    if (current_score1 > 0)
        winner_screen();
    return NULL;

}

void * listen_server(void* arg) {
    //TODO: Implement client disconnection
    //TODO: Create message transfer
    DATA* data = (DATA*) arg;

    /*for (int i = 1; i < 21; ++i) {
        pthread_mutex_lock(data->mut);
        data->server_buffer[i] = "fsgs";
        pthread_mutex_unlock(data->mut);
    }*/

    while (play == 1) {
        usleep(200000);
        char buff[256];
        bzero(buff, 256);
        n = read(sockt, buff, 255);

        pthread_mutex_lock(data->mut);
        if (data->writeIndex == 1 && data->count > 0) {
            pthread_cond_wait(data->can_write, data->mut);
            mvprintw(M + 6, 0, "Writer caka:                                      ");
        }

        data->server_buffer[data->writeIndex] = buff;

        mvprintw(M + 6, 0, "WRITER %s:    ",data->server_buffer[data->writeIndex]);
        mvprintw(M + 7, 0, "WRITER pise na index %d:                                                                              ",
                 data->writeIndex);
        if  (data->writeIndex++ == 21)
            data->writeIndex = 1;
        data->count++;
        pthread_cond_signal(data->can_read);

        pthread_mutex_unlock(data->mut);

    }
    return NULL;
}

/**
 * Initialization of snake
 */
void snake_init() {
    int j1 = x1;
    int j2 = x2;
    for (int i = 1; i <= head1; ++i) {
        field1[y_1][j1++] = head1 - (i - 1);
        field2[y2][++j2 - head2] = i;
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

    if (send_message("play") < 0) {
        endwin();
        exit(5);
    }

    attr_off(COLOR_PAIR(4),0);
    play = 1;

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

void set_values(char args[256]) {
    int i = 5;
    /*char * token = strtok(args, ";");
        while( token != NULL ) {
            switch (i) {
                case 0:
                    x2 = atoi(token);
                    break;
                case 1:
                    y2 = atoi(token);
                    break;
                case 2:
                    head2 = atoi(token);
                    break;
                case 3:
                    tail2 = atoi(token);
                    break;
                case 4:
                    current_score2 = atoi(token);
                    break;
                case 5:
                    fruit_x = atoi(token);
                    break;
                case 6:
                    fruit_y = atoi(token);
                    break;
                case 7:
                    game_status = atoi(token);
                    break;
                default:
                    mvprintw(M + 1, 0, "Something goes wrong!");
            }
            i++;
            token = strtok(NULL, ";");
        }*/
    mvprintw(M + 5, 0, args);

    /*char string[50];
    bzero(string, 50);
    strcpy(string, args);
    // Extract the first token
    char * token = strtok(string, ";");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        mvprintw(M + i, 0, " %s", token ); //printing each token
        i++;
        token = strtok(NULL, ";"); }
        */

}

int send_message(char *message) {
    bzero(buffer,256);
    strcpy(buffer, message);
    n = write(sockt, buffer, strlen(buffer));
    if (n < 0)
        return -1;
    bzero(buffer,256);
    return 0;
}