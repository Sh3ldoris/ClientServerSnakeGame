#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "Server.h"

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
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr, "usage %s port\n", argv[0]);
        return 1;
    } else {
        start_screen();
        connect_client(argv);
    }

    int **server_field;
    int **client_field;

    server_field = malloc(M * sizeof(int *));
    for (int i = 0; i < M; ++i)
        server_field[i] = malloc(N * sizeof(int));

    client_field = malloc(M * sizeof(int *));
    for (int i = 0; i < M; ++i)
        client_field[i] = malloc(N * sizeof(int));

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            server_field[i][j] = 0;
            client_field[i][j] = 0;
        }
    }

    /// Create threads
    DATA data = {
            server_field,
            client_field,
            5,
            5,
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
    pthread_cond_destroy(&cond2);
    pthread_cond_destroy(&cond3);
    pthread_mutex_destroy(&mut);

    for (int i = 0; i < M; ++i)
        free(data.field_server[i]);
    free(data.field_server);

    for (int i = 0; i < M; ++i)
        free(data.field_client[i]);
    free(data.field_client);

    close(newsockt);
    close(sockt);
    system("clear");
    endwin();

    return 0;
}

void connect_client(char *argv[]) {
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockt = socket(AF_INET, SOCK_STREAM, 0);
    if (sockt < 0) {
        perror("Error creating socket");
        endwin();
        exit(1);
    }

    if (bind(sockt, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket address");
        endwin();
        exit(2);
    }

    listen(sockt, 1);
    cli_len = sizeof(cli_addr);

    wait_opponent_join_screen();
    newsockt = accept(sockt, (struct sockaddr *) &cli_addr, &cli_len);
    if (newsockt < 0) {
        perror("Protivnikovi sa nepodarilo pripojit!");
        endwin();
        exit(3);
    }
    wait_opponent_to_start_game_screen();
}

void *client_communication(void *args) {
    DATA *data = (DATA *) args;

    int n;
    char buff[256];
    int field1server[M][N];
    int field2client[M][N];
    bzero(buff, 256);

    while (1) {
        n = read(newsockt, buff, 255);
        if (n < 0) {
            perror("Error reading to socket");
            break;
        }
        if (strcmp(buff, "play") == 0) {
            pthread_cond_broadcast(data->is_game_on);
            break;
        }
    }

    int drawn_already = 0;

    while (data->game_status == 3) {

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
        n = read(newsockt, &data->direction_change_client, sizeof(data->direction_change_client));
        if (n < 0) {
            perror("Error reading to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, field1server, sizeof(field1server));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, field2client, sizeof(field2client));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->head_server, sizeof(data->head_server));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->head_client, sizeof(data->head_client));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->score_server, sizeof(data->score_server));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->score_client, sizeof(data->score_client));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->fruit_x, sizeof(data->fruit_x));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->fruit_y, sizeof(data->fruit_y));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->fruit_value, sizeof(data->fruit_value));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->is_fruit_generated, sizeof(data->is_fruit_generated));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }
        usleep(1000);

        n = write(newsockt, &data->game_status, sizeof(data->game_status));
        if (n < 0) {
            perror("Error writing to socket");
            break;
        }

        data->is_drawn++;
        drawn_already = 1;

        pthread_mutex_unlock(data->mut);

        if (data->is_drawn == 2)
            pthread_cond_signal(data->can_read);
    }

    if (data->game_status == 3) {
        pthread_mutex_lock(data->mut);
        data->game_status = 0;
        data->is_drawn++;
        pthread_mutex_unlock(data->mut);

        if (data->is_drawn == 2)
            pthread_cond_signal(data->can_read);
    }

    return NULL;
}

void *handle_server_player(void *arg) {
    DATA *data = (DATA *) arg;

    pthread_mutex_lock(data->mut);
    if (data->game_status == 3) {
        pthread_cond_wait(data->is_game_on, data->mut);
    }
    pthread_mutex_unlock(data->mut);

    int c = 0;
    int drawn_already = 0;
    int was_countdown = 1;
    int pressed_x = 0;

    draw_arena();

    while(data->game_status == 3) {
        c = getch(); /// Get input

        pthread_mutex_lock(data->mut);

        if (data->is_drawn == 2 || drawn_already == 1) {
            pthread_cond_wait(data->can_draw, data->mut);
        }
        drawn_already = 0;

        /// Check input
        if (c != ERR) {
            if (c == 97)
                data->direction_change_server = 4;
            if (c == 100)
                data->direction_change_server = 2;
            if (c == 120) {
                pressed_x = 1;
                data->direction_change_server = 0;
            }
            if (c == 119)
                data->direction_change_server = 1;
            if (c == 115)
                data->direction_change_server = 3;
        }

        for (int i = 1; i <= M - 1; i++) {
            for (int j = 1; j <= N - 1; j++) {
                if ((data->field_server[i][j] > 0) && (data->field_server[i][j] < data->head_server)) {
                    attron(COLOR_PAIR(5));
                    mvprintw(i, j, "o");
                    attroff(COLOR_PAIR(5));
                } else if ((data->field_client[i][j] > 0) && (data->field_client[i][j] < data->head_client)) {
                    attron(COLOR_PAIR(4));
                    mvprintw(i, j, "o");
                    attroff(COLOR_PAIR(4));
                }
                else if (data->field_server[i][j] == data->head_server) {
                    attron(COLOR_PAIR(5));
                    mvprintw(i, j, "x");
                    attroff(COLOR_PAIR(5));
                }else if (data->field_client[i][j] == data->head_client) {
                    attron(COLOR_PAIR(4));
                    mvprintw(i, j, "x");
                    attr_off(COLOR_PAIR(4), 0 );
                }
                else if (data->is_fruit_generated == 1 && j == data->fruit_x && i == data->fruit_y && was_countdown == 1) {
                    mvprintw(i, j, "%d", data->fruit_value);
                } else {
                    mvprintw(i, j, " ");
                }
            }
        }

        mvprintw(M + 2, (N / 2) - 17, "Your Score: %d  Opponent's Score: %d", data->score_server, data->score_client);
        move(M + 3, 0);

        data->is_drawn++;
        drawn_already = 1;

        pthread_mutex_unlock(data->mut);

        /*if (was_countdown == 0) {
            was_countdown = 1;
            drawn_already = 0;
            for (int i = 3; i > 0; --i) {
                attr_on(COLOR_PAIR(3),0);
                mvprintw(M / 2, (N/2), "%d", i);
                move(M + 1,0);
                attr_off(COLOR_PAIR(3),0);
                refresh();
                sleep(1);
            }
        }*/

        if (data->is_drawn == 2) {
            pthread_cond_signal(data->can_read);
        }

        refresh();
    }

    switch (data->game_status) {
        case 1:
            draw_game_over();
            sleep(2);
            winner_screen();
            break;
        case 2:
            draw_game_over();
            sleep(2);
            loser_screen();
            break;
        default:
            if (pressed_x == 1) {
                you_left_screen();
            } else
                opponent_left_screen();
            system("clear");
            sleep(1);
            break;
    }

    return NULL;
}

void *handle_game(void *arg) {
    DATA *data = (DATA *) arg;

    pthread_mutex_lock(data->mut);
    if (data->game_status == 3) {
        pthread_cond_wait(data->is_game_on, data->mut);
    }
    pthread_mutex_unlock(data->mut);

    int tail1 = 1;
    int y_1 = M / 4;
    int x1 = 10;

    int tail2 = 1;
    int y2 = (M / 4) * 3 + 1;
    int x2 = N - 10;

    int direction_server = 2;
    int direction_client = 4;

    /// Inicialization of snakes in fields
    int j1 = x1;
    int j2 = x2;
    for (int i = 1; i <= data->head_server; ++i) {
        data->field_server[y_1][++j1 - data->head_server] = i;
        data->field_client[y2][j2++] = data->head_client - (i - 1);
    }

    while (data->game_status == 3) {
        pthread_mutex_lock(data->mut);

        if (data->is_drawn < 2) {
            pthread_cond_wait(data->can_read, data->mut);
        }

        if (data->is_fruit_generated == 0) {
            data->fruit_x = (rand() % (N - 2)) + 1;
            data->fruit_y = (rand() % (M - 2)) + 1;

            data->fruit_value = (rand() % 3) + 1;

            /// Pokial sa vygeneruje napr v tele hada tak sa nezobrazi
            if ((data->field_server[data->fruit_y][data->fruit_x] == 0) ||
                (data->field_client[data->fruit_y][data->fruit_x] == 0))
                data->is_fruit_generated = 1;
        }

        switch (data->direction_change_server) {
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
                data->game_status = 0;
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

        switch (data->direction_change_client) {
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
                data->game_status = 0;
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
        if (data->is_fruit_generated == 1) {
            if (data->fruit_x == x1 && data->fruit_y == y_1) {
                data->is_fruit_generated = 0;
                for (int i = 0; i < data->fruit_value; ++i) {
                    tail1--;
                    data->score_server++;
                }
            }
            if (data->fruit_x == x2 && data->fruit_y == y2) {
                data->is_fruit_generated = 0;
                for (int i = 0; i < data->fruit_value; ++i) {
                    tail2--;
                    data->score_client++;
                }
            }
        }

        /// Collision check
        if ((data->field_server[y_1][x1] != 0) || (data->field_client[y_1][x1] != 0)) {
            data->game_status = 2;
        }

        if ((data->field_client[y2][x2] != 0) || (data->field_server[y2][x2] != 0)) {
            data->game_status = 1;
        }

        //// Step
        data->field_server[y_1][x1] = ++data->head_server;
        data->field_client[y2][x2] = ++data->head_client;

        /// shift tail
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < N; ++j) {
                if (data->field_server[i][j] == tail1)
                    data->field_server[i][j] = 0;
                if (data->field_client[i][j] == tail2)
                    data->field_client[i][j] = 0;
            }
        }
        tail1++;
        tail2++;

        if (data->score_server >= winning_score)
            data->game_status = 1;
        if (data->score_client >= winning_score)
            data->game_status = 2;

        data->is_drawn = 0;

        pthread_mutex_unlock(data->mut);

        pthread_cond_broadcast(data->can_draw);

        if (data->game_status == 3) {
            usleep(200000);
        }
    }
    refresh();

    return NULL;
}

void draw_arena() {
    move(0, 0);
    attron(COLOR_PAIR(2));
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
    attroff(COLOR_PAIR(2));
    refresh();
}

void draw_game_over() {
    attron(COLOR_PAIR(3));
    mvprintw((M / 2) - 1, (N / 2) - 6, "           ");
    mvprintw(M / 2, (N / 2) - 6, " Game OVER ");
    mvprintw((M / 2) + 1, (N / 2) - 6, "           ");
    move(0, 0);
    attroff(COLOR_PAIR(3));
    move(M + 1, 0);
    refresh();
}

void countdown() {
    for (int i = 3; i > 0; --i) {
        attron(COLOR_PAIR(3));
        mvprintw(M / 2, (N / 2), "%d", i);
        move(M + 1, 0);
        attroff(COLOR_PAIR(3));
        refresh();
        sleep(1);
    }
}

void start_screen() {
    system("clear");
    draw_arena();
    attron(COLOR_PAIR(1));
    mvprintw(M / 2 - 4, N / 2 - 20, "    ________         ________  ");
    mvprintw(M / 2 - 3, N / 2 - 20, "   /        \\       /        \\        0 ");
    mvprintw(M / 2 - 2, N / 2 - 20, "  /  /----\\  \\     /  /----\\  \\       0  ");
    mvprintw(M / 2 - 1, N / 2 - 20, "  |  |    |  |     |  |    |  |      / \\ ");
    mvprintw(M / 2, N / 2 - 20, "  |  |    |  |     |  |    |  |     /  |  ");
    mvprintw(M / 2 + 1, N / 2 - 20, " (o  o)   \\   \\___/   /    \\   \\___/   / ");
    mvprintw(M / 2 + 2, N / 2 - 20, "  \\__/     \\         /      \\         /  ");
    mvprintw(M / 2 + 3, N / 2 - 20, "            --------         -------- 	");
    attroff(COLOR_PAIR(1));

    mvprintw(M / 2 + 5, N / 2 - 12, "Start when you are ready!");
    refresh();

    attron(COLOR_PAIR(4));
    while (getch() != '\n')  /// caka na enter
    {
        mvprintw(M / 2 + 6, N / 2 - 10, "Press ENTER to START.");
        move(M + 1, 0);
        refresh();
        sleep(1);
        mvprintw(M / 2 + 6, N / 2 - 10, "                     ");
        move(M + 1, 0);
        refresh();
        sleep(1);

    }
    attroff(COLOR_PAIR(4));
}

void wait_opponent_join_screen() {
    draw_arena();

    attron(COLOR_PAIR(1));
    mvprintw(M / 2 - 4, N / 2 - 10, "     /");
    mvprintw(M / 2 - 3, N / 2 - 10, "   \\/ ");
    mvprintw(M / 2 - 2, N / 2 - 18, "  .... ->->->->->");
    mvprintw(M / 2 - 1, N / 2 - 18, "  |  |            ");
    mvprintw(M / 2, N / 2 - 18, "  |  |                ");
    mvprintw(M / 2 + 1, N / 2 - 18, " (o  o)             ");
    mvprintw(M / 2 + 2, N / 2 - 18, "  \\__/           ");
    mvprintw(M / 2 + 3, N / 2 - 18, "      	");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(3));
    mvprintw(M / 2 - 4, N / 2 + 5, " \\/");
    mvprintw(M / 2 - 3, N / 2 + 5, " /\\ ");
    mvprintw(M / 2 - 2, N / 2 + 2, "<-<-<-<-<- .... ");
    mvprintw(M / 2 - 1, N / 2 + 2, "           |  |");
    mvprintw(M / 2, N / 2 + 2, "           |  |   ");
    mvprintw(M / 2 + 1, N / 2 + 2, "          (o  o)   ");
    mvprintw(M / 2 + 2, N / 2 + 2, "           \\__/   ");
    mvprintw(M / 2 + 3, N / 2 + 2, "      	");
    attroff(COLOR_PAIR(3));

    attron(COLOR_PAIR(4));
    mvprintw(M / 2 + 6, N / 2 - 15, "Waiting for OPPONENT to join...");
    move(M + 2, 0);
    refresh();
    attroff(COLOR_PAIR(4));
}

void wait_opponent_to_start_game_screen() {
    //draw_arena();
    attron(COLOR_PAIR(1));
    mvprintw(M / 2 - 4, N / 2 - 10, "     /            /");
    mvprintw(M / 2 - 3, N / 2 - 10, "   \\/          \\/ ");
    mvprintw(M / 2 - 2, N / 2 - 18, "  .... ----------------------- .... ");
    mvprintw(M / 2 - 1, N / 2 - 18, "  |  |                         |  |");
    mvprintw(M / 2, N / 2 - 18, "  |  |                         |  |   ");
    mvprintw(M / 2 + 1, N / 2 - 18, " (o  o)                       (o  o)   ");
    mvprintw(M / 2 + 2, N / 2 - 18, "  \\__/                         \\__/   ");
    attroff(COLOR_PAIR(1));
    mvprintw(M / 2 + 4, N / 2 - 15, " Opponent JOINED SUCCESSFULLY.");

    attron(COLOR_PAIR(4));
    mvprintw(M / 2 + 6, N / 2 - 18, "Waiting for OPPONENT to START GAME...");
    move(M + 2, 0);
    refresh();
    usleep(100000);

    attroff(COLOR_PAIR(4));
}

void loser_screen() {
    draw_arena();
    attron(COLOR_PAIR(3));
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
    attroff(COLOR_PAIR(3));

    mvprintw(M / 2 + 4, N / 2 - 11, "	    Game OVER!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attron(COLOR_PAIR(1));
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attroff(COLOR_PAIR(1));
}

void winner_screen() {
    draw_arena();
    attron(COLOR_PAIR(4));
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
    attroff(COLOR_PAIR(4));

    mvprintw(M / 2 + 4, N / 2 - 10, "       You WON!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attron(COLOR_PAIR(1));
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attroff(COLOR_PAIR(1));
}

void opponent_left_screen() {
    //system("clear");
    draw_arena();
    attron(COLOR_PAIR(3));
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
    attroff(COLOR_PAIR(3));

    mvprintw(M / 2 + 4, N / 2 - 13, "Ooops! Something went WRONG!");
    mvprintw(M / 2 + 5, N / 2 - 5, "We 're SORRY!");
    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attron(COLOR_PAIR(1));
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attroff(COLOR_PAIR(1));
}

void you_left_screen() {
    //system("clear");
    draw_arena();
    attron(COLOR_PAIR(3));
    mvprintw(M / 2 - 7, N / 2 - 10, "    /-----------\\     ");
    mvprintw(M / 2 - 6, N / 2 - 10, "   /             \\  ");
    mvprintw(M / 2 - 5, N / 2 - 10, "  /               \\  ");
    mvprintw(M / 2 - 4, N / 2 - 10, " |     O     O     |  ");
    mvprintw(M / 2 - 3, N / 2 - 10, " |                 | ");
    mvprintw(M / 2 - 2, N / 2 - 10, " |     _______     |");
    mvprintw(M / 2 - 1, N / 2 - 10, "  \\   /       \\   / ");
    mvprintw(M / 2, N / 2 - 10, "   \\             / ");
    mvprintw(M / 2 + 1, N / 2 - 10, "    \\___________/");
    attroff(COLOR_PAIR(3));

    mvprintw(M / 2 + 4, N / 2 - 9, "You LEFT the GAME!");
    mvprintw(M / 2 + 5, N / 2 - 12, "We HOPE you'll come BACK!");

    mvprintw(M + 2, (N / 2) - 17, "                                     ");
    refresh();

    attron(COLOR_PAIR(1));
    while (getch() != '\n') {
        mvprintw(M / 2 + 6, N / 2 - 13, "  Press ENTER to FINISH !");
        move(M + 1, 0);
    }
    attroff(COLOR_PAIR(1));
}