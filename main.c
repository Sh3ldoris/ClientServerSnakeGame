#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define N 50
#define M 20

int field[20][50] = {0};

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
            if (i == 0 || i == M) {
                printf("#");
            } else if (j == 0 || j == N) {
                printf("#");
            } else if (field[i][j] == 1) {
                printf("o");
            } else
                printf(" ");
        }
        printf("\n");
    }

    printf("\n   Current Score: %d  HighScore: %d",12, 100);
    printf("\n");
}


void snake_init() {
    field[1][1] = 1;
}

void up() {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field[i][j] == 1) {
                field[i][j] = 0;
                field[i-1][j] = 1;
                return;
            }
        }
    }
}

void down() {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field[i][j] == 1) {
                field[i][j] = 0;
                field[i+1][j] = 1;
                return;
            }
        }
    }
}

void left() {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field[i][j] == 1) {
                field[i][j] = 0;
                field[i][j-1] = 1;
                return;
            }
        }
    }
}

void right() {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (field[i][j] == 1) {
                field[i][j] = 0;
                field[i][j+1] = 1;
                return;
            }
        }
    }
}



void main() {
    int c;

    snake_init();

    while(1) {
        set_mode(1);

        system("clear");
        print();

        while (!(c = get_key())) usleep(10000);
        if (c != 0) {
            if (c == 97)
                left();
            if (c == 100)
                right();
            if (c == 120)
                break;
            if (c == 119)
                up();
            if (c == 115)
                down();
        }

        //sleep(1);
    }
    /*field[1][1] = 1;
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 50; ++j) {
            printf("%d", field[i][j]);
        }
        printf("\n");
    }*/

}
