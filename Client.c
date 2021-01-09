#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include "Client.h"

int main(int argc, char *argv[]) {


    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char *) server->h_addr,
            (char *) &serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 3;
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to socket");
        return 4;
    }

    signal(SIGINT, signal_callback_handler);

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
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

    srand(time(NULL));


    start_screen();

    bzero(buffer, 256);
    strcpy(buffer, "play");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
        perror("Error writing to socket");
        return 5;
    }

    int c = 0;
    int direction_change = 4;
    int was_countdown = 0;
    int pressed_x = 0;

    draw_arena();

    while(game_status == 3) {

        /// Get input
        if (key_hit()) {
            c = getch();
            if (c == 97)
                direction_change = 4;
            if (c == 100)
                direction_change = 2;
            if (c == 120) {
                pressed_x = 1;
                direction_change = 0;
            }
            if (c == 119)
                direction_change = 1;
            if (c == 115)
                direction_change = 3;
        }

        usleep(100000);

        n = write(sockfd, &direction_change, sizeof(direction_change));
        if (n < 0) {
            perror("Error writing from socket");
            return 6;
        }

        n = read(sockfd, &field2, sizeof(field2));
        if (n < 0) {
            perror("Error reading from socket");
            return 6;
        }

        n = read(sockfd, &field1, sizeof(field1));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        n = read(sockfd, &head2, sizeof(head2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        n = read(sockfd, &head1, sizeof(head1));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        n = read(sockfd, &current_score2, sizeof(current_score2));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        n = read(sockfd, &current_score1, sizeof(current_score1));
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
        }

        n = read(sockfd, &fruit_generated, sizeof(fruit_generated));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        n = read(sockfd, &game_status, sizeof(game_status));
        if (n < 0) {
            perror("Error writing to socket");
            return 6;
        }

        draw_game();

        if (was_countdown == 0) {
            was_countdown = 1;
            for (int i = 3; i > 0; --i) {
                attr_on(COLOR_PAIR(3),0);
                mvprintw(M / 2, (N/2), "%d", i);
                move(M + 1,0);
                attr_off(COLOR_PAIR(3),0);
                refresh();
                sleep(1);
            }
        }
    }

    close(sockfd);

    switch (game_status) {
        case 2:
            draw_game_over();
            sleep(2);
            winner_screen();
            break;
        case 1:
            draw_game_over();
            sleep(2);
            loser_screen();
            break;
        default:
            if (pressed_x == 1) {
                you_left_screen();
            } else
                opponent_left_screen();
            break;
    }
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

void signal_callback_handler (int signum) {
    close(sockfd);
    endwin();
    exit(signum);
}

void draw_game() {
    for (int i = 1; i <= M - 1; i++) {
        for (int j = 1; j <= N - 1; j++) {
            if ((field2[i][j] > 0) && (field2[i][j] < head2))  {
                attr_on(COLOR_PAIR(5), 0);
                mvprintw(i, j, "o");
                attr_off(COLOR_PAIR(5), 0);
            } else if ((field1[i][j] > 0) && (field1[i][j] < head1)) {
                attr_on(COLOR_PAIR(4), 0);
                mvprintw(i, j, "o");
                attr_off(COLOR_PAIR(4), 0);
            }
            else if (field2[i][j] == head2)  {
                attr_on(COLOR_PAIR(5), 0);
                mvprintw(i, j, "x");
                attr_off(COLOR_PAIR(5), 0);
            } else if (field1[i][j] == head1) {
                attr_on(COLOR_PAIR(4), 0);
                mvprintw(i, j, "x");
                attr_off(COLOR_PAIR(4), 0);
            }
            else if (fruit_generated == 1 && j == fruit_x && i == fruit_y) {
                mvprintw(i, j, "%d", fruit_value);
            } else {
                mvprintw(i, j, " ");
            }
        }
    }

    mvprintw(M + 2, (N / 2) - 17, "Your Score: %d  Opponent's Score: %d", current_score1, current_score2);
    move(M + 3, 0);

    refresh();
}

void draw_arena() {
    move(0, 0);
    attr_on(COLOR_PAIR(2), 0);
    for (int i = 0; i <= M; i++) {
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
    attr_off(COLOR_PAIR(2), 0);
    refresh();
}

void draw_game_over() {
    attr_on(COLOR_PAIR(3), 0);
    mvprintw((M / 2) - 1, (N / 2) - 6, "           ");
    mvprintw(M / 2, (N / 2) - 6, " Game OVER ");
    mvprintw((M / 2) + 1, (N / 2) - 6, "           ");
    move(0, 0);
    attr_off(COLOR_PAIR(3), 0);
    move(M + 1, 0);
    refresh();
}

void countdown() {
    //draw_game();
    for (int i = 3; i > 0; --i) {
        attr_on(COLOR_PAIR(3), 0);
        mvprintw(M / 2, (N / 2), "%d", i);
        move(M + 1, 0);
        attr_off(COLOR_PAIR(3), 0);
        refresh();
        sleep(1);
    }
}

void start_screen() {
    system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(1), 0);
    mvprintw(M / 2 - 4, N / 2 - 20, "    ________         ________  ");
    mvprintw(M / 2 - 3, N / 2 - 20, "   /        \\       /        \\        0 ");
    mvprintw(M / 2 - 2, N / 2 - 20, "  /  /----\\  \\     /  /----\\  \\       0  ");
    mvprintw(M / 2 - 1, N / 2 - 20, "  |  |    |  |     |  |    |  |      / \\ ");
    mvprintw(M / 2, N / 2 - 20, "  |  |    |  |     |  |    |  |     /  |  ");
    mvprintw(M / 2 + 1, N / 2 - 20, " (o  o)   \\   \\___/   /    \\   \\___/   / ");
    mvprintw(M / 2 + 2, N / 2 - 20, "  \\__/     \\         /      \\         /  ");
    mvprintw(M / 2 + 3, N / 2 - 20, "            --------         -------- 	");
    attr_off(COLOR_PAIR(1), 0);

    mvprintw(M / 2 + 5, N / 2 - 12, "Start when you are ready!");
    refresh();

    attr_on(COLOR_PAIR(4), 0);
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 10, "Press ENTER to START.");
        move(M + 1, 0);
        refresh();
        sleep(1);
        mvprintw(M / 2 + 6, N / 2 - 10, "                     ");
        move(M + 1, 0);
        refresh();
        sleep(1);
    }

    attr_off(COLOR_PAIR(4), 0);

}

void loser_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3), 0);
    mvprintw(M / 2 - 7, N / 2 - 11, "         ________   ");
    mvprintw(M / 2 - 6, N / 2 - 11, "        /        \\  ");
    mvprintw(M / 2 - 5, N / 2 - 11, "       /  /----\\  \\  ");
    mvprintw(M / 2 - 4, N / 2 - 11, "       |  |    |  |  ");
    mvprintw(M / 2 - 3, N / 2 - 11, "       |  |    |  | ");
    mvprintw(M / 2 - 2, N / 2 - 11, "      (o  o)   |  | ");
    mvprintw(M / 2 - 1, N / 2 - 11, "  |\\   \\__/    |  | ");
    mvprintw(M / 2, N / 2 - 11, "  | \\__________/  / ");
    mvprintw(M / 2 + 1, N / 2 - 11, "  \\              / ");
    mvprintw(M / 2 + 2, N / 2 - 11, "   -------------  	");
    attr_off(COLOR_PAIR(3), 0);

    mvprintw(M / 2 + 4, N / 2 - 11, "	    Game OVER!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attr_on(COLOR_PAIR(1), 0);
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1), 0);
}

void winner_screen() {
    draw_arena();
    attr_on(COLOR_PAIR(4), 0);
    mvprintw(M / 2 - 7, N / 2 - 22, "      _________________________________  ");
    mvprintw(M / 2 - 6, N / 2 - 22, "     /                                 \\ ");
    mvprintw(M / 2 - 5, N / 2 - 22, "    /  /-----------------------------\\  \\ ");
    mvprintw(M / 2 - 4, N / 2 - 22, "    |  |          . . . . . . .      |  | ");
    mvprintw(M / 2 - 3, N / 2 - 22, "    |  |          |\\/\\/\\/\\/\\/\\|      |  | ");
    mvprintw(M / 2 - 2, N / 2 - 22, "   (o  o)         | o o o o o |      |  | ");
    mvprintw(M / 2 - 1, N / 2 - 22, "    \\__/    |\\    |___________|      |  | 	");
    mvprintw(M / 2, N / 2 - 22, "            | \\______________________/  /  ");
    mvprintw(M / 2 + 1, N / 2 - 22, "            \\                          /  ");
    mvprintw(M / 2 + 2, N / 2 - 22, "             --------------------------  ");
    attr_off(COLOR_PAIR(4), 0);

    mvprintw(M / 2 + 4, N / 2 - 10, "       You WON!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attr_on(COLOR_PAIR(1), 0);
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1), 0);

}

void opponent_left_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3), 0);
    mvprintw(M / 2 - 7, N / 2 - 10, "         / \\   ");
    mvprintw(M / 2 - 6, N / 2 - 10, "        /   \\  ");
    mvprintw(M / 2 - 5, N / 2 - 10, "       /     \\");
    mvprintw(M / 2 - 4, N / 2 - 10, "      /   _   \\ ");
    mvprintw(M / 2 - 3, N / 2 - 10, "     /   | |   \\ ");
    mvprintw(M / 2 - 2, N / 2 - 10, "    /    | |    \\ ");
    mvprintw(M / 2 - 1, N / 2 - 10, "   /     |_|     \\ ");
    mvprintw(M / 2, N / 2 - 10, "  /               \\ ");
    mvprintw(M / 2 + 1, N / 2 - 10, " /        O        \\ ");
    mvprintw(M / 2 + 2, N / 2 - 10, "/___________________\\ ");
    attr_off(COLOR_PAIR(3), 0);

    mvprintw(M / 2 + 4, N / 2 - 20, "Ooops! Looks like your opponent LEFT !");
    mvprintw(M / 2 + 5, N / 2 - 5, "We 're SORRY!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attr_on(COLOR_PAIR(1), 0);
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1), 0);
}

void you_left_screen() {
    //system("clear");
    draw_arena();
    attr_on(COLOR_PAIR(3), 0);
    mvprintw(M / 2 - 7, N / 2 - 10, "    /-----------\\     ");
    mvprintw(M / 2 - 6, N / 2 - 10, "   /             \\  ");
    mvprintw(M / 2 - 5, N / 2 - 10, "  /               \\  ");
    mvprintw(M / 2 - 4, N / 2 - 10, " |     O     O     |  ");
    mvprintw(M / 2 - 3, N / 2 - 10, " |                 | ");
    mvprintw(M / 2 - 2, N / 2 - 10, " |     _______     |");
    mvprintw(M / 2 - 1, N / 2 - 10, "  \\   /       \\   / ");
    mvprintw(M / 2, N / 2 - 10, "   \\             / ");
    mvprintw(M / 2 + 1, N / 2 - 10, "    \\___________/");
    attr_off(COLOR_PAIR(3), 0);

    mvprintw(M / 2 + 4, N / 2 - 9, "You LEFT the GAME!");
    mvprintw(M / 2 + 5, N / 2 - 12, "We HOPE you'll come BACK!");

    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attr_on(COLOR_PAIR(1), 0);
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attr_off(COLOR_PAIR(1), 0);
}