#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

/// Rozmery plochy
#define N 50
#define M 15

int field[15][50] = {0}; /// Pozicia pre hraca 1
int direction = 2; /// Smer pohybu (1-4 clockwise)
int head = 5;
int tail = 1;
int y = 1;
int x = 6;

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
            } else if (field[i][j] == head)
                printf("}");
            else
                printf(" ");
        }
        printf("\n");
    }

    printf("\n   Current Score: %d  HighScore: %d",12, 100);
    printf("\n");
}


void snake_init() {
    int j = x;
    for (int i = 0; i < head; ++i) {
        field[y][++j - head] = i + 1;
    }
}


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
    int c = 0;
    int direction_change = 2;
    //TODO: 1. Init game menu
    play = 1;

    snake_init();

    while(play) {
        /// Clear windows
        system("clear");

        /// Print game
        print();

        /// Snake step
        step(direction_change);

        printf("Direction --> %d\n", direction);
        printf("Head --> %d\n", head);
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

        usleep(300000);
    }

    return 0;
}
