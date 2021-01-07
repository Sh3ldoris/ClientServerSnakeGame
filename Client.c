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

int fruit_generated = 1;
int fruit_x = 10;
int fruit_y = 7;
int fruit_value = 0;

int play = 1;
int game_status = 0;

int sockt, n;
int sockfd;
struct sockaddr_in serv_addr;
struct hostent* server;

char buffer[256];

void draw_arena();
int key_hit();
void connect_to_server(char *argv[]);
void play_game();
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
void something_went_wrong_screen();
void get_info();
void send_info();
void set_values(char args[256]);
int send_message(char *message);

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 3;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to socket");
        return 4;
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
    bzero(buffer,256);
    strcpy(buffer,"play");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
    {
        perror("Error writing to socket");
        return 5;
    }



    int c = 0;
    int direction_change = direction;

    snake_init();
    draw_arena();

    /// Countdown from 3
    //countdown();


    //get_info();

    while(play) {
        /*bzero(buffer,256);
        n = read(sockfd, buffer, 255);
        if (n < 0)
        {
            perror("Error reading from socket");
            return 6;
        }

        mvprintw(M+4, 0, "Udaje zo servera: %s", buffer);*//*
        int n = 0;
        n = read(sockfd, &x2, sizeof(x2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &y2, sizeof(y2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &head2, sizeof(head2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        field2[y2][x2] = head2;

        n = read(sockfd, &tail2, sizeof(tail2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &current_score2, sizeof(current_score2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &fruit_x, sizeof(fruit_x));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &fruit_y, sizeof(fruit_y));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }
        n = read(sockfd, &fruit_value, sizeof(fruit_value));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }*/


        /// 1. Nastav udaje - zavolaj GET na server
        //get_info();
        /// 2. Posli udaje o sebe na server
        //send_info();

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

        mvprintw(M + 1, 45, "P1 : P2");
        mvprintw(M + 2, 45, "%d : %d ", x1,x2);
        mvprintw(M + 3, 45, "%d : %d ",y_1,y2);
        mvprintw(M + 4, 45, "%d : %d",head1,head2);
        mvprintw(M + 5, 45, "%d : %d",tail1,tail2);
        mvprintw(M + 6, 45, "%d : %d",current_score1,current_score2);
        mvprintw(M + 7, 45, "%d",fruit_x);
        mvprintw(M + 8, 45, "%d",fruit_y);
        mvprintw(M + 9, 45, "%d",game_status);

        step(direction_change);

        /// Print play_game area
        draw_game();



        usleep(200000);
    }
    refresh();

    draw_game_over();

    sleep(2);

    /// CHANGE TO SWITCH
    if (current_score1 == 0)
        loser_screen();
    if (current_score1 > 0)
        winner_screen();
    /*switch (game_stat) {
        case 1:
            winner_screen();
            break;
        case 2:
            loser_screen();
            break;
        default:
            something_went_wrong_screen();
            break;
    }*/



    close(sockfd);
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

void play_game() {

    int c = 0;
    int direction_change = direction;

    snake_init();
    draw_arena();

    /// Countdown from 3
    //countdown();


    //get_info();

    while(play) {

        /// 1. Nastav udaje - zavolaj GET na server
        //get_info();
        /// 2. Posli udaje o sebe na server
        //send_info();

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

        eat_fruit();
        check_collision();
        usleep(300000);
    }
    refresh();

    draw_game_over();

    sleep(2);

    if (current_score1 == 0)
        loser_screen();
    if (current_score1 > 0)
        winner_screen();

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

    field1[y_1][x1] = ++head1;
    field2[y2][x2] = ++head2;

    /// shift tail1
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field1[i][j] == tail1)
                field1[i][j] = 0;
            if (field2[i][j] == tail2)
                field2[i][j] = 0;
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

    /*if (send_message("play") < 0) {
        endwin();
        exit(5);
    }
*/
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

void something_went_wrong_screen() {
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

void get_info() {

    bzero(buffer,256);
    strcpy(buffer, "getinfo");
    n = write(sockt, buffer, strlen(buffer));
    bzero(buffer, strlen(buffer));
    mvprintw(M + 8, 0, "Som tu");
    //usleep(1000);

    n = read(sockt, buffer, strlen(buffer));
    mvprintw(M + 9, 0, "Odpoved: %d   ", n);

    /*if (n > 0) {
        //TODO: set info
        mvprintw(M + 10, 0, "Odpoved servera: %s   ", buffer);
        break;
    }*/
    mvprintw(M + 10, 0, "Odpoved servera: %s   ", buffer);
    mvprintw(M + 11, 0, "Koniec GET requestu   ");
}

void send_info() {
    char info[256];
    bzero(info, strlen(info));
    sprintf(info, "%d;%d;%d;%d;%d;%d;%d;%d", x1, y_1, head1, tail1, current_score1, game_status);
    send_message(info);
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