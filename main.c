#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>


/// Rozmery plochy
#define N  50
#define M  15

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

void set_mode(int want_key)
{
    static struct termios old, new;
    if (!want_key) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        return;
    }

    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

int get_key()
{
    int c = 0;
    struct timeval tv;
    fd_set fs;
    tv.tv_usec = tv.tv_sec = 0;

    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    select(STDIN_FILENO + 1, &fs, 0, 0, &tv);

    if (FD_ISSET(STDIN_FILENO, &fs)) {
        c = getchar();
        set_mode(0);
    }
    return c;
}

void print() {
    for(int i=0;i<=M;i++){
        for (int j = 0; j <= N; ++j) {
            if (i == 0 || i == M || j == 0 || j == N) {
                printf("#");
            } else if ((field[i][j] >= tail) && (field[i][j] < head)) {
                printf("o");
            } else if (field[i][j] == head) {
                printf("x");
            } else if (fruit_generated == 1 && j == fruit_x && i == fruit_y) {
                printf("%d",fruit_value);
            } else {
                printf(" ");
            }
            /// !!! Bacha na ELSE vetvu !!!
        }
        printf("\n");
    }


    printf("\n   Current Score: %d  HighScore: %d",current_score, 100);
    printf("\n");
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

int main() {
    srand(time(NULL));
    int c = 0;
    int direction_change = 2;

    //TODO: 1. Init game menu
    play = 1;
    snake_init();

    while(play) {
        /// Clear windows
        system("clear");

        /// Generate fruit
        if (fruit_generated == 0) {
            generate_fruit();
        }

        /// Print game
        print();

        printf("Direction --> %d\n", direction);
        printf("Head x,y--> [%d,%d]\n", x, y);
        printf("Fruit x,y--> [%d,%d]\n", fruit_x, fruit_y);
        printf("Tail --> %d\n", tail);
        printf("Body --> %d\n", head - tail);

        /// Get input
        set_mode(1);
        c = get_key();
        if (c != 0) {
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

    //TODO: End screen menu

    return 0;
}
