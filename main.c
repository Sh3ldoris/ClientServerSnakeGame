#include <stdio.h>
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
int y = 1;
int x = 6;
int current_score = 0;

int fruit_generated = 0;
int fruit_x = 10;
int fruit_y = 7;
int fruit_value = 0;

int play = 0;

void draw_area();
int key_hit();
void draw_game();
void snake_init();
void draw_area();
void draw_game_over();
void generate_fruit();
void eat_fruit();
void check_collision();
void step(int change);
void start_screen();
void loser_screen();
void winner_screen();

int main() {

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    srand(time(NULL));
    int c = 0;
    int direction_change = 2;

    start_screen();

    snake_init();
    draw_area();

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
    system("clear");

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


/**
 * Initialization of snake
 */
void snake_init() {
    int j = x;
    for (int i = 0; i < head; ++i) {
        field[y][++j - head] = i + 1;
    }
}

void draw_area() {
    move(0,0);
    for(int i=0;i<=M;i++){
        for (int j = 0; j <= N; ++j) {
            if (i == 0 || i == M || j == 0 || j == N) {
                printw("#");
            } else {
                printw(" ");
            }
            /// !!! Bacha na ELSE vetvu !!!
        }
        printw("\n");
    }
    refresh();
}

void draw_game_over() {
    mvprintw((M / 2) - 1, (N/2) - 6, "           ");
    mvprintw(M / 2, (N/2) - 6, " Game OVER ");
    mvprintw((M / 2) + 1, (N/2) - 6, "           ");
    move(0,0);
    refresh();
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
    move(0,0);
    printw("\n");
    printw("    ________         ________ 			  \n");
    printw("   /        \\       /        \\        0  \n");
    printw("  /  /----\\  \\     /  /----\\  \\       0  \n");
    printw("  |  |    |  |     |  |    |  |      / \\ \n");
    printw("  |  |    |  |     |  |    |  |     /  |  \n");
    printw(" (o  o)   \\   \\___/   /    \\   \\___/   / \n");
    printw("  \\__/     \\         /      \\         /  \n");
    printw("            --------         -------- 	   \n");
    printw("\n");
    printw("	Start when you are ready!\n");
    printw("	  Press ENTER to START.	\n");

    refresh();

    while (getch() != '\n')  /// caka na enter
    play = 1;
    system("clear");
}

void loser_screen() {
    system("clear");
    move(0,0);
    printw("\n");
    printw("         ________    		\n");
    printw("        /        \\     		\n");
    printw("       /  /----\\  \\   		\n");
    printw("       |  |    |  |    	\n");
    printw("       |  |    |  |   	\n");
    printw("      (o  o)   |  | 	\n");
    printw("  |\\   \\__/    |  | 	\n");
    printw("  | \\__________/  /    	\n");
    printw("  \\              /    	\n");
    printw("   -------------  		\n");
    printw("	      Game OVER!\n");
    printw("	    Your SCORE: %d !\n", current_score);
    printw("\n");
    refresh();

    while (getch() != '\n');
}

void winner_screen() {
    system("clear");
    move(0,0);
    printw("\n");
    printw("         _________________________________    		\n");
    printw("        /                                 \\     		\n");
    printw("       /  /-----------------------------\\  \\   		\n");
    printw("       |  |          . . . . . . .      |  |    	\n");
    printw("       |  |          |\\/\\/\\/\\/\\/\\|      |  |   	\n");
    printw("      (o  o)         | o o o o o |      |  | 	\n");
    printw("       \\__/    |\\    |___________|      |  | 	\n");
    printw("               | \\______________________/  /    	\n");
    printw("               \\                          /    	\n");
    printw("                --------------------------  		\n");
    printw("	       You WON!\n");
    printw("	    Your SCORE: %d !\n", current_score);

    printw("\n");
    refresh();
    while (getch() != '\n');

}
