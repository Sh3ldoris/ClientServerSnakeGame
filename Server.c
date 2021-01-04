#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>

/// Rozmery plochy
#define N  50
#define M  18

int field[M][N] = {0}; /// Pozicia pre hraca 1
int direction = 2; /// Smer pohybu (1-4 clockwise)
int head = 5;
int tail = 1;
int y = M / 4;
int x = 10;
int current_score = 0;

int fruit_generated = 0;
int fruit_x = 10;
int fruit_y = 7;
int fruit_value = 0;

int play = 0;

void draw_arena();
int key_hit();
void draw_game();
void snake_init();
void draw_arena();
void draw_game_over();
void countdown();
void generate_fruit();
void eat_fruit();
void check_collision();
void step(int change);
void start_screen();
void loser_screen();
void winner_screen();
void wait_opponent_join_screen();
void wait_opponent_to_start_game_screen();

int main() {

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
    int c = 0;
    int direction_change = 2;

    start_screen();

    snake_init();
    draw_arena();

    /// Countdown from 3
    countdown();

    while(play) {
        /// Generate fruit
        if (fruit_generated == 0) {
            generate_fruit();
        }

        /// Print game area
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

        /// Snake step
        step(direction_change);

        usleep(300000);
    }
    refresh();

    draw_game_over();

    sleep(2);

    if (current_score == 0)
        loser_screen();
    if (current_score > 0)
        winner_screen();


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

/**
 * Initialization of snake
 */
void snake_init() {
    int j = x;
    for (int i = 0; i < head; ++i) {
        field[y][++j - head] = i + 1;
    }
}

void draw_game() {
    for(int i = 1; i <= M - 1; i++){
        for (int j = 1; j <= N - 1; j++) {
             if ((field[i][j] >= tail) && (field[i][j] < head)) {
                mvprintw(i, j, "o");
            } else if (field[i][j] == head) {
                 mvprintw(i, j, "x");
            } else if (fruit_generated == 1 && j == fruit_x && i == fruit_y) {
                 mvprintw(i, j, "%d",fruit_value);
            } else {
                 mvprintw(i, j, " ");
            }
            /// !!! Bacha na ELSE vetvu !!!
        }
    }
    mvprintw(M + 2, (N/2) - 16, "Current Score: %d  HighScore: %d",current_score, 100);
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
    if (field[fruit_y][fruit_x] == 0)
        fruit_generated = 1;
}

/**
 * Checks if snake is at fruit position
 */
void eat_fruit(){
    if ((fruit_x) == x && fruit_y == y) {
        fruit_generated = 0;
        for (int i = 0; i < fruit_value; ++i) {
            tail--;
            current_score++;
        }
    }
}

/**
 * Checks snake collision with itself
 */
void check_collision() {
    if (field[y][x] != 0)
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
            if (y-- <= 1)
                y = M - 1;
            break;
        case 2:
            if (x++ >= N - 1)
                x = 1;
            break;
        case 3:
            if (y++ >= M - 1)
                y = 1;
            break;
        case 4:
            if (x-- <= 1)
                x = N - 1;
            break;
        default:
            break;
    }

    eat_fruit();
    check_collision();

    field[y][x] = ++head;

    /// shift tail
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field[i][j] == tail)
                field[i][j] = 0;
        }
    }
    tail++;
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

    wait_opponent_join_screen();
}

void wait_opponent_join_screen() {
    //system("clear");
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

    attr_on(COLOR_PAIR(4),0);
    while (getch() != '\n')  /// treba doplnit logiku ze ked dostane od supera "play" zacne
    {
        mvprintw(M/2 + 6,  N/2 - 15 , "Waiting for OPPONENT to join.  ");
        move(M + 1, 0);
        refresh();
        usleep(100000);
        mvprintw(M/2 + 6,  N/2 - 15 , "Waiting for OPPONENT to join.. ");
        move(M + 1, 0);
        refresh();
        usleep(100000);
        mvprintw(M/2 + 6,  N/2 - 15 , "Waiting for OPPONENT to join...");
        move(M + 1, 0);
        refresh();
        usleep(100000);

    }
    attr_off(COLOR_PAIR(4),0);
    wait_opponent_to_start_game_screen();
}

void wait_opponent_to_start_game_screen() {
    draw_arena();
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

    attr_on(COLOR_PAIR(4),0);
    while (getch() != '\n')  /// treba doplnit logiku ze ked dostane od supera "play" zacne
    {
        mvprintw(M/2 + 6,  N/2 - 18 , "Waiting for OPPONENT to START GAME.  ");
        move(M + 1, 0);
        refresh();
        usleep(100000);
        mvprintw(M/2 + 6,  N/2 - 18 , "Waiting for OPPONENT to START GAME.. ");
        move(M + 1, 0);
        refresh();
        usleep(100000);
        mvprintw(M/2 + 6,  N/2 - 18 , "Waiting for OPPONENT to START GAME...");
        move(M + 1, 0);
        refresh();
        usleep(100000);


    }
    attr_off(COLOR_PAIR(4),0);
    play = 1;
};

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
    mvprintw(M/2 + 5,  N/2 - 11,"	 Your SCORE: %d !", current_score);

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
    mvprintw(M/2 + 5, N/2 - 10,"  Your SCORE: %d !", current_score);

    mvprintw(M + 2, (N/2) - 16, "                                    ");
    refresh();

    attr_on(COLOR_PAIR(1),0);
    while (getch() != '\n') {
        mvprintw(M/2 + 6, N/2 - 13,"  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1),0);

}
