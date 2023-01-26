#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>


#define MAP_WIDTH 21
#define MAP_HEIGHT 21
//#define MAP_WIDTH 10
//#define MAP_HEIGHT 3
#define SERVER_IP "127.0.0.1"
#define PORT 8595
WINDOW * menuwin;
WINDOW * score_menu;
WINDOW * legend;
WINDOW * command;
WINDOW * command;
pthread_t *thread_id = NULL;
pthread_t thread_id_player[4];
pthread_t thread_id_key[4];
pthread_t thread_id_player_listen;
pthread_attr_t tattr;
pthread_t input;
int PLAYERS = 0;
int BEASTS = 0;
int DROPS = 0;
int EXIT = 0;
int TREASURES = 0;
sem_t sem,timer_sema,get_key_sema_main;
int Exit_4=0;
int server_fd;
struct sockaddr_in address;
int opt = 1;
int new_socket;
char **map = NULL;
int time_game = 0;
int campsite_x=12;
int campsite_y=9;
//char *map_backup[MAP_HEIGHT]={
//        "##########\n",
//        "#        #\n",
//        "##########\n"
//};
char *map_backup[MAP_HEIGHT] = {
        "#####################\n",
        "#       #         # #\n",
        "#####&# # #####&### #\n",
        "# #  &# #         & #\n",
        "# # ##### ######### #\n",
        "#       #   #   #   #\n",
        "# # ########### ### #\n",
        "# # #     #     #  &#\n",
        "# # ### # ##### ###&#\n",
        "#&# #   #   A       #\n",
        "# # ########### ### #\n",
        "# # # # #   #   # # #\n",
        "# ### # ### ### # ###\n",
        "#     # # #     # # #\n",
        "# ### # # ### ### # #\n",
        "# #     &&        # #\n",
        "# ##### # # # ### # #\n",
        "# #     # # # # #   #\n",
        "# ####### # ### ### #\n",
        "#     #   #       # #\n",
        "#####################\n"};


struct Drop {
    int x;
    int y;
    int coins;
};
struct Player {
    int x;
    int y;
    int coins;
    int score;
    char symbol;
    int socket;
    int slow;
    int way;
    char map_5x5[5][5];
    int dead;
    int dc;
    pthread_t th;
    sem_t send_maze_semaphore;
    pthread_t th_send_maze;
    pthread_t th_get_key;
    sem_t move_position_sema;
    sem_t get_key_sema;
};
struct Beast {
    sem_t move_beast_semaphore;
    int x;
    int y;
    int way;
    pthread_t th;
};
void display_maze();
void update_map();
struct Treasure {
    int x;
    int y;
    char symbol;
};
struct Treasure **L_Treasures = NULL;
struct Beast **L_Beasts = NULL;
struct Player *L_Players[4] = {NULL};
struct Drop **L_Drops = NULL;

void *random_position(int *x, int *y) {
    int tmp_x = rand() % MAP_WIDTH;
    int tmp_y = rand() % MAP_HEIGHT;
    while (map[tmp_y][tmp_x] != ' ') {
        tmp_x = rand() % MAP_WIDTH;
        tmp_y = rand() % MAP_HEIGHT;
    }
    *y = tmp_y;
    *x = tmp_x;
};


void move_position(struct Player *p1, int way) {

    if (p1 == NULL)
        return;
    switch (way) {
        case 's': {
            if (p1->y + 1 < MAP_HEIGHT && map[p1->y + 1][p1->x] != '#') {
                if (map[p1->y + 1][p1->x] == '&' && p1->slow != 's') {
                    p1->slow = 's';
                    break;
                }
                p1->slow = 0;
                p1->way = 's';
                map[p1->y][p1->x] = ' ';
                if(map[p1->y+1][p1->x]=='*'||isdigit((map[p1->y+1][p1->x])))
                    p1->dead=1;
                p1->y++;
            }
            break;
        }
        case 'd': {
            if (p1->x + 1 < MAP_WIDTH && map[p1->y][p1->x + 1] != '#') {
                if (map[p1->y][p1->x + 1] == '&' && p1->slow != 'd') {
                    p1->slow = 'd';
                    break;
                }
                p1->slow = 0;
                p1->way = 'd';
                map[p1->y][p1->x] = ' ';
                if(map[p1->y][p1->x+1]=='*'|| isdigit((map[p1->y][p1->x+1])))
                    p1->dead=1;
                p1->x++;
            }
            break;

        }
        case 'w': {
            if (p1->y - 1 >= 0 && map[p1->y - 1][p1->x] != '#') {
                if (map[p1->y - 1][p1->x] == '&' && p1->slow != 'w') {
                    p1->slow = 'w';
                    break;
                }
                p1->slow = 0;
                p1->way = 'w';
                map[p1->y][p1->x] = ' ';
                if(map[p1->y-1][p1->x]=='*'||isdigit((map[p1->y-1][p1->x])))
                    p1->dead=1;
                p1->y--;
            }
            break;
        }
        case 'a': {
            if (p1->x - 1 >= 0 && map[p1->y][p1->x - 1] != '#') {
                if (map[p1->y][p1->x - 1] == '&' && p1->slow != 'a') {
                    p1->slow = 'a';
                    break;
                }
                p1->slow = 0;
                p1->way = 'a';
                map[p1->y][p1->x] = ' ';
                if(map[p1->y][p1->x-1]=='*'||isdigit((map[p1->y][p1->x-1])))
                    p1->dead=1;
                p1->x--;
            }
            break;
        }
        default:
            return;
    }


}
void *ai_beast(struct Beast *B) ;
void *add_treasures(char symbol) {
    for (int i = 0; i < TREASURES; ++i) {
        if(L_Treasures[i]==NULL)
        {
            L_Treasures[i] = malloc(sizeof(struct Treasure));
            if(L_Treasures[i]==NULL)
            {
                EXIT=1;
                return NULL;
            }
            random_position(&L_Treasures[i]->x, &L_Treasures[i]->y);
            L_Treasures[i]->symbol = symbol;
            return L_Treasures[i];
        }
    }
    TREASURES++;
    L_Treasures = realloc(L_Treasures, sizeof(struct Treasure) * TREASURES);
    if(L_Treasures==NULL)
    {
        EXIT=1;
        return NULL;
    }
    L_Treasures[TREASURES - 1] = malloc(sizeof(struct Treasure));
    if(L_Treasures[TREASURES - 1]==NULL)
    {
        EXIT=1;
        return NULL;
    }
    random_position(&L_Treasures[TREASURES - 1]->x, &L_Treasures[TREASURES - 1]->y);
    L_Treasures[TREASURES - 1]->symbol = symbol;
    return L_Treasures[TREASURES - 1];

}

void *del_treasures(int x, int y) {
    for (int i = 0; i < TREASURES; ++i) {
        if(L_Treasures[i]==NULL)
            continue;
        if (L_Treasures[i]->x == x && L_Treasures[i]->y == y) {
            free(L_Treasures[i]);
            L_Treasures[i] = NULL;
            break;
        }
    }
}

void *add_drops(const int *x, const int *y, const int *coins) {
    if (coins == NULL)
        return NULL;
    int tmp_x;
    int tmp_y;
    if (x == NULL || y == NULL) {
        tmp_x = rand() % MAP_WIDTH;
        tmp_y = rand() % MAP_HEIGHT;
        while (map[tmp_y][tmp_x] != ' ' && map[tmp_y][tmp_x] != '*' && map[tmp_y][tmp_x] != '#' &&
               !isdigit(map[tmp_y][tmp_x]) && map[tmp_y][tmp_x] != '&') {
            tmp_x = rand() % MAP_WIDTH;
            tmp_y = rand() % MAP_HEIGHT;
        }
    }
    tmp_x = *x;
    tmp_y = *y;
    struct Drop *flaga = NULL;
    for (int i = 0; i < DROPS; ++i) {
        if (L_Drops[i] == NULL) {
            L_Drops[i] = malloc(sizeof(struct Drop));
            if(L_Drops[i]==NULL)
            {
                EXIT=1;
                return NULL;
            }
            L_Drops[i]->x = tmp_x;
            L_Drops[i]->y = tmp_y;
            L_Drops[i]->coins = *coins;
            flaga = L_Drops[i];
        }
    }
    if (flaga == NULL) {
        DROPS++;
        L_Drops = realloc(L_Drops, sizeof(struct Drop *) * DROPS);
        if(L_Drops==NULL)
        {
            EXIT=1;
            return NULL;
        }
        L_Drops[DROPS - 1] = malloc(sizeof(struct Drop));
        if(L_Drops[DROPS - 1]==NULL)
        {
            EXIT=1;
            return NULL;
        }
        L_Drops[DROPS - 1]->x = tmp_x;
        L_Drops[DROPS - 1]->y = tmp_y;
        L_Drops[DROPS - 1]->coins = *coins;
        flaga = L_Drops[DROPS - 1];
    }
    return flaga;
}

void *delete_drops(struct Drop *d) {
    if (d == NULL)
        return NULL;

    for (int i = 0; i < DROPS; ++i) {
        if (d->x == L_Drops[i]->x && d->y == L_Drops[i]->y) {
            free(L_Drops[i]);
            L_Drops[i] = NULL;
        }
    }
}

void *update_player(struct Player *P) {
    if (P == NULL) {
        pthread_exit(NULL);
    }
    for (int j = 0; j < BEASTS; ++j) {
        if (L_Beasts[j] == NULL)
            continue;
        int abs_x = abs(P->x - L_Beasts[j]->x);
        int abs_y = abs(P->y - L_Beasts[j]->y);
        double distance = ceil(sqrt(abs_x * abs_x + abs_y * abs_y));
        if ((P->y == L_Beasts[j]->y && P->x == L_Beasts[j]->x) ||
            (distance == 1 && P->way == 'w' && L_Beasts[j]->way == 's') ||
            (distance == 1 && P->way == 'a' && L_Beasts[j]->way == 'd') ||
            (distance == 1 && P->way == 'd' && L_Beasts[j]->way == 'a') ||
            (distance == 1 && P->way == 's' && L_Beasts[j]->way == 'w')) {
            if (P->coins > 0) {

                int drop_coins = P->coins;
                add_drops(&L_Beasts[j]->x, &L_Beasts[j]->y, &drop_coins);
                P->coins = 0;

            }
            random_position(&P->x, &P->y);
        }

    }
    for (int j = 0; j < DROPS; ++j) {
        if (L_Drops[j] == NULL)
            continue;
        if (P->y == L_Drops[j]->y && P->x == L_Drops[j]->x) {
            P->coins += L_Drops[j]->coins;
            delete_drops(L_Drops[j]);
        }
    }
    for (int j = 0; j < PLAYERS; ++j) {
        if (P->symbol == L_Players[j]->symbol)
            continue;
        int abs_x = abs(P->x - L_Players[j]->x);
        int abs_y = abs(P->y - L_Players[j]->y);
        double distance = ceil(sqrt(abs_x * abs_x + abs_y * abs_y));
        if ((P->y == L_Players[j]->y && P->x == L_Players[j]->x) ||
            (distance == 1 && P->way == 'w' && L_Players[j]->way == 's') ||
            (distance == 1 && P->way == 'a' && L_Players[j]->way == 'd') ||
            (distance == 1 && P->way == 'd' && L_Players[j]->way == 'a') ||
            (distance == 1 && P->way == 's' && L_Players[j]->way == 'w')) {
            if (P->coins > 0 || (L_Players[j]->coins > 0)) {
                int drop_coins = P->coins + L_Players[j]->coins;
                add_drops(&P->x, &P->y, &drop_coins);
                P->coins = 0;
                L_Players[j]->coins = 0;
            }
            map[P->y][P->x] = ' ';
            map[L_Players[j]->y][L_Players[j]->x] = ' ';
            random_position(&P->x, &P->y);
            random_position(&L_Players[j]->x, &L_Players[j]->y);
        }
    }
    if (map[P->y][P->x] == 'A') {
        P->score += P->coins;
        P->coins = 0;
    }
    if (map[P->y][P->x] == 'C') {
        P->coins++;
    }
    if (map[P->y][P->x] == 'T') {
        P->coins += 50;
    }
    if (map[P->y][P->x] == 't') {
        P->coins += 10;
    }
    map[P->y][P->x] = P->symbol;
}

void update_map() {
    sem_wait(&sem);
    for (int i=0;i<4;i++)
    {
        if(L_Players[i]==NULL)
            continue;
        sem_wait(&L_Players[i]->get_key_sema);
        sem_post(&L_Players[i]->get_key_sema);

    }
//    for (int i = 0; i < BEASTS; ++i) {
//        sem_wait(&L_Beasts[i]->move_beast_semaphore);
//        //sem_post(&L_Beasts[i]->move_beast_semaphore);
//    }
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if (map_backup[i][j] == '&') {
                map[i][j] = '&';
            }
            if (map_backup[i][j] == 'A') {
                map[i][j] = 'A';
            }
        }
    }
    for (int i = 0; i < TREASURES; ++i) {
        if(L_Treasures[i]==NULL)
            continue;
        map[L_Treasures[i]->y][L_Treasures[i]->x]=L_Treasures[i]->symbol;
    }
    for (int i = 0; i < BEASTS; ++i) {
        map[L_Beasts[i]->y][L_Beasts[i]->x] = '*';
    }
    for (int i = 0; i < 4; ++i) {
        if (L_Players[i] == NULL)
            continue;
        //map[L_Players[i]->y][L_Players[i]->x]=L_Players[i]->symbol;
        for (int j = 0; j < BEASTS; ++j) {
            if (L_Beasts[j] == NULL)
                continue;
            int abs_x = abs(L_Players[i]->x - L_Beasts[j]->x);
            int abs_y = abs(L_Players[i]->y - L_Beasts[j]->y);
            double distance = ceil(sqrt(abs_x * abs_x + abs_y * abs_y));
            if ((L_Players[i]->y == L_Beasts[j]->y && L_Players[i]->x == L_Beasts[j]->x) ||
                (distance == 1 && L_Players[i]->way == 'w' && L_Beasts[j]->way == 's') ||
                (distance == 1 && L_Players[i]->way == 'a' && L_Beasts[j]->way == 'd') ||
                (distance == 1 && L_Players[i]->way == 'd' && L_Beasts[j]->way == 'a') ||
                (distance == 1 && L_Players[i]->way == 's' && L_Beasts[j]->way == 'w'||L_Players[i]->dead==1)
                    ) {
                if (L_Players[i]->coins > 0) {

                    int drop_coins = L_Players[i]->coins;
                    add_drops(&L_Beasts[j]->x, &L_Beasts[j]->y, &drop_coins);
                    L_Players[i]->coins = 0;

                }
                //printf("DEad\n");
                L_Players[i]->dead=0;
                //map[L_Players[i]->y][L_Players[i]->x]='*';
                random_position(&L_Players[i]->x, &L_Players[i]->y);
            }

        }
        for (int j = 0; j < DROPS; ++j) {
            if (L_Drops[j] == NULL)
                continue;
            if (L_Players[i]->y == L_Drops[j]->y && L_Players[i]->x == L_Drops[j]->x) {
                L_Players[i]->coins += L_Drops[j]->coins;
                delete_drops(L_Drops[j]);
            }
        }
        for (int j = 0; j < 4; ++j) {
            if(L_Players[j]==NULL)
                continue;
            if (L_Players[i]->symbol == L_Players[j]->symbol)
                continue;
            int abs_x = abs(L_Players[i]->x - L_Players[j]->x);
            int abs_y = abs(L_Players[i]->y - L_Players[j]->y);
            double distance = ceil(sqrt(abs_x * abs_x + abs_y * abs_y));
            if ((L_Players[i]->y == L_Players[j]->y && L_Players[i]->x == L_Players[j]->x) ||
                (distance == 1 && L_Players[i]->way == 'w' && L_Players[j]->way == 's') ||
                (distance == 1 && L_Players[i]->way == 'a' && L_Players[j]->way == 'd') ||
                (distance == 1 && L_Players[i]->way == 'd' && L_Players[j]->way == 'a') ||
                (distance == 1 && L_Players[i]->way == 's' && L_Players[j]->way == 'w')||(L_Players[i]->dead==1&&L_Players[j]->dead==1)) {
                if (L_Players[i]->coins > 0 || (L_Players[j]->coins > 0)) {
                    int drop_coins = L_Players[i]->coins + L_Players[j]->coins;
                    add_drops(&L_Players[i]->x, &L_Players[i]->y, &drop_coins);
                    L_Players[i]->coins = 0;
                    L_Players[j]->coins = 0;
                }
                L_Players[i]->dead=0;
                L_Players[j]->dead=0;
                //printf("DEad\n");
                map[L_Players[i]->y][L_Players[i]->x] = ' ';
                map[L_Players[j]->y][L_Players[j]->x] = ' ';
                random_position(&L_Players[i]->x, &L_Players[i]->y);
                random_position(&L_Players[j]->x, &L_Players[j]->y);
            }
        }
        if (map[L_Players[i]->y][L_Players[i]->x] == 'A') {
            L_Players[i]->score += L_Players[i]->coins;
            L_Players[i]->coins = 0;
        }
        if (map[L_Players[i]->y][L_Players[i]->x] == 'C') {
            L_Players[i]->coins++;
        }
        if (map[L_Players[i]->y][L_Players[i]->x] == 'T') {
            L_Players[i]->coins += 50;
        }
        if (map[L_Players[i]->y][L_Players[i]->x] == 't') {
            L_Players[i]->coins += 10;
        }
        del_treasures(L_Players[i]->x,L_Players[i]->y);

    }
    for (int i = 0; i < DROPS; ++i) {
        if(L_Drops[i]==NULL)
            continue;
        map[L_Drops[i]->y][L_Drops[i]->x] = 'D';
    }
    for (int i = 0; i < 4; ++i) {
        if(L_Players[i]==NULL)
            continue;
        map[L_Players[i]->y][L_Players[i]->x] = L_Players[i]->symbol;
    }
    sem_post(&sem);


    //display_maze();

}

void *add_Beast() {
    sem_wait(&sem);
    int tmp_x = rand() % MAP_WIDTH;
    int tmp_y = rand() % MAP_HEIGHT;
    while (map[tmp_y][tmp_x] != ' ' && map[tmp_y][tmp_x] != '*' && map[tmp_y][tmp_x] != '1') {
        tmp_x = rand() % MAP_WIDTH;
        tmp_y = rand() % MAP_HEIGHT;
    }
    BEASTS++;
    L_Beasts = realloc(L_Beasts, sizeof(struct Beast *) * BEASTS);
    if(L_Beasts==NULL)
    {
        EXIT=1;
        sem_post(&sem);
        return NULL;
    }
    thread_id = realloc(thread_id, sizeof(pthread_t) * BEASTS);
    if(thread_id==NULL)
    {
        EXIT=1;
        sem_post(&sem);
        return NULL;
    }
    L_Beasts[BEASTS - 1] = malloc(sizeof(struct Beast));
    if(L_Beasts[BEASTS-1]==NULL)
    {
        EXIT=1;
        sem_post(&sem);
        return NULL;
    }
    L_Beasts[BEASTS - 1]->x = tmp_x;
    L_Beasts[BEASTS - 1]->y = tmp_y;
    L_Beasts[BEASTS - 1]->way = 0;
    sem_init(&L_Beasts[BEASTS - 1]->move_beast_semaphore,0,0);
    sem_post(&sem);
    return L_Beasts[BEASTS - 1];
}

void display_maze() {

    init_pair(1,COLOR_WHITE,COLOR_WHITE);
    init_pair(2,COLOR_RED,COLOR_BLACK);
    init_pair(3,COLOR_BLACK,COLOR_MAGENTA);
    init_pair(4,COLOR_BLACK,COLOR_YELLOW);
    init_pair(5,COLOR_BLACK,COLOR_GREEN);
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if(map[i][j]=='#')
            {
                wattron(menuwin,COLOR_PAIR(1));
                mvwprintw(menuwin,i+1,j+1,"#");
                wattroff(menuwin,COLOR_PAIR(1));
                continue;
            }
            else if(map[i][j]=='&')
            {
                mvwprintw(menuwin,i+1,j+1,"#");
                continue;
            }
            else if(map[i][j]=='*')
            {
                wattron(menuwin,COLOR_PAIR(2));
                mvwprintw(menuwin,i+1,j+1,"*");
                wattroff(menuwin,COLOR_PAIR(2));
                continue;
            }
            else if(isdigit(map[i][j]))
            {
                wattron(menuwin,COLOR_PAIR(3));
                mvwprintw(menuwin,i+1,j+1,"%c",map[i][j]);
                wattroff(menuwin,COLOR_PAIR(3));
                continue;
            }
            else if(map[i][j]=='C'||map[i][j]=='T'||map[i][j]=='t') {
                wattron(menuwin, COLOR_PAIR(4));
                mvwprintw(menuwin, i+1, j+1, "%c", map[i][j]);
                wattroff(menuwin, COLOR_PAIR(4));
                continue;
            }
            else if(map[i][j]=='A') {
                wattron(menuwin, COLOR_PAIR(5));
                mvwprintw(menuwin, i+1, j+1, "%c", map[i][j]);
                wattroff(menuwin, COLOR_PAIR(5));
                continue;
            }else
                mvwprintw(menuwin,i+1,j+1,"%c",map[i][j]);
        }
    }

    mvwprintw(score_menu,1,1,"Server Info IP: %s",SERVER_IP);
    if(campsite_x==-1||campsite_y==-1)
        mvwprintw(score_menu,2,2,"Campsite X/Y: ?/?");
    else
        mvwprintw(score_menu,2,2,"Campsite X/Y: %d/%d",campsite_x,campsite_y);
    mvwprintw(score_menu,3,2,"Tick: %d",time_game);
    mvwprintw(score_menu,4,3,"Player: ");
    mvwprintw(score_menu,5,3,"Position: ");
    mvwprintw(score_menu,7,3,"Carried money: ");
    mvwprintw(score_menu,8,3,"Brought money: ");
    for (int i = 0; i < 4; ++i) {
        if(L_Players[i]==NULL)
        {
            mvwprintw(score_menu,4,(i+2)*10,"?");
            mvwprintw(score_menu,5,(i+2)*10,"? ?");
            mvwprintw(score_menu,7,(i+2)*10,"?");
            mvwprintw(score_menu,8,(i+2)*10,"?");
        }
        else
        {
            mvwprintw(score_menu,4,(i+2)*10,"%d",(L_Players[i]->symbol-'0'));
            mvwprintw(score_menu,5,(i+2)*10,"%d %d",L_Players[i]->x,L_Players[i]->y);
            mvwprintw(score_menu,7,(i+2)*10,"%d",L_Players[i]->coins);
            mvwprintw(score_menu,8,(i+2)*10,"%d",L_Players[i]->score);
        }

    }
    for(int i=0;i<PLAYERS;i++)
    {
        if(L_Players[i]==NULL)
            continue;
        wattron(menuwin,COLOR_PAIR(3));
        mvwprintw(menuwin,L_Players[i]->y+1,L_Players[i]->x+1,"%c",L_Players[i]->symbol);
        wattroff(menuwin,COLOR_PAIR(3));
    }
    for(int i=0;i<BEASTS;i++)
    {
        if(L_Beasts[i]==NULL)
            continue;
        wattron(menuwin,COLOR_PAIR(2));
        mvwprintw(menuwin,L_Beasts[i]->y+1,L_Beasts[i]->x+1,"*");
        wattroff(menuwin,COLOR_PAIR(2));
    }
    mvwprintw(legend,1,1,"Type: q to quit");
    mvwprintw(legend,2,1,"Type: keypad to move");
    mvwprintw(legend,3,1,"* - beast");
    mvwprintw(legend,4,1,"1 2 3 4 - players");
    mvwprintw(legend,5,1,"t - 10 coins");
    mvwprintw(legend,6,1,"C - 1 coin");
    mvwprintw(legend,7,1,"T - 50 coins");

    wrefresh(menuwin);


};

void *add_player() {
    int tmp_x = rand() % MAP_WIDTH;
    int tmp_y = rand() % MAP_HEIGHT;
    while (map[tmp_y][tmp_x] != ' ') {
        tmp_x = rand() % MAP_WIDTH;
        tmp_y = rand() % MAP_HEIGHT;
    }
    PLAYERS++;
    char tmp;
    int whcich = 0;
    for (int i = 0; i < 4; ++i) {
        if (L_Players[i] == NULL) {
            tmp = (char) (i + 1) + '0';
            whcich = i;
            break;
        }
    }
    *(L_Players + whcich) = malloc(sizeof(struct Player));
    if(*(L_Players + whcich)==NULL)
    {
        EXIT=1;
        return NULL;
    }
    L_Players[whcich]->x = tmp_x;
    L_Players[whcich]->y = tmp_y;
    L_Players[whcich]->coins = 0;
    L_Players[whcich]->score = 0;
    L_Players[whcich]->slow = 0;
    L_Players[whcich]->symbol = tmp;
    L_Players[whcich]->dead = 0;
    L_Players[whcich]->dc = 0;
    L_Players[whcich]->way = 'w';
    L_Players[whcich]->socket = 0;
    sem_init(&L_Players[whcich]->send_maze_semaphore,0,0);
    sem_init(&L_Players[whcich]->get_key_sema,0,1);
    sem_init(&L_Players[whcich]->move_position_sema,0,1);
    return L_Players[whcich];
}

void *delete_player(char symbol) {
    sem_wait(&sem);
    for (int i = 0; i < 4; ++i) {
        if (L_Players[i] != NULL && L_Players[i]->symbol == symbol) {
            close(L_Players[i]->socket);
            //sem_destroy(&L_Players[i]->get_key_sema);
            L_Players[i]->dc=1;
            sem_post(&L_Players[i]->send_maze_semaphore);

            pthread_join(L_Players[i]->th_get_key,NULL);

            pthread_join(L_Players[i]->th_send_maze,NULL);
            map[L_Players[i]->y][L_Players[i]->x] = ' ';
            free(*(L_Players + i));
            L_Players[i] = NULL;
            PLAYERS--;
            sem_post(&sem);
            sem_destroy(&L_Players[i]->send_maze_semaphore);
            sem_destroy(&L_Players[i]->get_key_sema);
            return NULL;

        }
    }
    sem_post(&sem);
    return NULL;
}

void *create_5x5(struct Player *p) {
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            p->map_5x5[i][j] = '\0';
        }
    }
    for (int i = -2, h = 0; i < 3; ++i, ++h) {
        if (p->y + i < 0 || p->y + i >= MAP_HEIGHT)
            continue;
        for (int j = -2, z = 0; j < 3; ++j, ++z) {
            if (p->x + j < 0 || p->x + j >= MAP_WIDTH)
                continue;
            p->map_5x5[h][z] = map[p->y + i][p->x + j];
        }

    }
}

void *send_maze(struct Player *p) {
    while(1) {
        if(EXIT==1)
            pthread_exit(NULL);
        sem_wait(&p->send_maze_semaphore);
        if(p->dc==1)
            pthread_exit(NULL);
        sem_wait(&sem);
        create_5x5(p);
        int h = send(p->socket, &p->x, sizeof(int), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        h = send(p->socket, &p->y, sizeof(int), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        h = send(p->socket, &p->coins, sizeof(int), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        h = send(p->socket, &p->score, sizeof(int), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        h = send(p->socket, &p->symbol, sizeof(char), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        h = send(p->socket, &time_game, sizeof(int), MSG_NOSIGNAL);
        if (h == -1) {
            sem_post(&sem);
            pthread_exit(NULL);

        }
        for (int i = 0; i < 5; ++i) {
            h = send(p->socket, p->map_5x5[i], 5, MSG_NOSIGNAL);
            if (h == -1) {
                sem_post(&sem);
                pthread_exit(NULL);

            }

        }
        sem_post(&sem);
    }
}

void *get_key(struct Player *p) {
    while (1)
    {

        if (EXIT == 1)
            pthread_exit(NULL);
        int tmp = -1;
        int h = read(p->socket, &tmp, sizeof(int));
        if (h == 0 && PLAYERS != 0) {
            delete_player(p->symbol);
            sem_post(&p->get_key_sema);
            pthread_exit(NULL);
        }

        if (tmp == 'a' || tmp == 'd' || tmp == 's' || tmp == 'w' && h != -1) {
            sem_wait(&sem);
            move_position(p, tmp);
            sem_post(&sem);
        }
        else
        {
            sem_wait(&sem);
            p->way = 0;
            sem_post(&sem);
        }

    }
    sem_post(&p->get_key_sema);


}

void *move_beast(struct Beast *B) {
//    while(1) {
    if(EXIT==1)
        pthread_exit(NULL);
    //sem_wait(&B->move_beast_semaphore);
    int way;
    int flag = 0;
    while (flag != 1) {
        way = rand() % 4;
        switch (way) {
            case 0: {
                if (B->y + 1 >= MAP_HEIGHT || map[B->y + 1][B->x] == '#')
                    break;
                map[B->y][B->x] = ' ';
                B->y++;
                B->way = 'w';
                flag = 1;
                break;
            }
            case 1: {
                if (B->x + 1 >= MAP_WIDTH || map[B->y][B->x + 1] == '#')
                    break;
                map[B->y][B->x] = ' ';
                B->x++;
                B->way = 'd';
                flag = 1;
                break;
            }
            case 2: {
                if (B->y - 1 <= 0 || map[B->y - 1][B->x] == '#')
                    break;
                map[B->y][B->x] = ' ';
                B->y--;
                B->way = 's';
                flag = 1;
                break;
            }
            case 3: {
                if (B->x - 1 <= 0 || map[B->y][B->x - 1] == '#')
                    break;
                map[B->y][B->x] = ' ';
                B->x--;
                B->way = 'a';
                flag = 1;
                break;
            }
            default:
                return NULL;
        }
    }
    map[B->y][B->x] = ' ';
//    }
};

void *input_key() {
    int key = 0;
    while (1) {
        key = getchar();
        mvwprintw(command,1,1,"You pressed %c",key);
        if (key == 'b' || key == 'B') {

            struct Beast *tmp=add_Beast();
            pthread_create(&tmp->th,NULL,(void*)ai_beast,tmp);

        }
        if (key == 'T') {
            add_treasures('T');
        }
        if (key == 'C') {
            add_treasures('C');
        }
        if (key == 't') {
            add_treasures('t');
        }
        if (key == 'q' || key == 'Q') {
            EXIT = 1;
            pthread_exit(NULL);
        }
        usleep(700000);
    }
}
void *timer() {
    while(1)
    {
        sleep(1);
        sem_wait(&sem);
        time_game++;
        sem_post(&sem);
        sem_post(&timer_sema);
        if(EXIT==1)
            pthread_exit(NULL);
    }
}
void *listening(void * y) {

    while (1) {

        if (EXIT == 1) {
            EXIT = 1;
            sem_post(&sem);
            pthread_exit(NULL);
        }

        struct sockaddr_in client;
        socklen_t clilen = sizeof(client);
        if ((new_socket = accept(server_fd, (struct sockaddr *) &client,
                                 &clilen)) >= 0) {
            sem_wait(&sem);
            int width = MAP_WIDTH;
            int height = MAP_HEIGHT;
            int h = send(new_socket, &height, sizeof(int), MSG_NOSIGNAL);
            if (h == -1) {
                sem_post(&sem);
                pthread_exit(NULL);

            }
            h = send(new_socket, &width, sizeof(int), MSG_NOSIGNAL);
            if (h == -1) {
                sem_post(&sem);
                pthread_exit(NULL);

            }
            if (PLAYERS == 4) {
                //EXIT = 1;
                close(new_socket);
                sem_post(&sem);
                continue;
                //pthread_exit(NULL);
            }

            struct Player *pom = add_player();
            pom->socket = new_socket;
            pthread_create(&pom->th, NULL, (void *) get_key, pom);
            pthread_create(&pom->th_send_maze, NULL, (void *) send_maze, pom);
            sem_post(&sem);
        }
    }
}


void *ai_beast(struct Beast *B) {
    //pthread_t beast;
    while (1) {
        if (EXIT == 1)
            pthread_exit(NULL);
        sem_wait(&B->move_beast_semaphore);
        sem_wait(&sem);
        if (BEASTS == 0)
        {
            sem_post(&sem);
            return NULL;
        }

        int index = -1;
        int abs_x;
        int abs_y;
        double distance = -1;
        double min = MAP_WIDTH * MAP_HEIGHT;
        for (int i = 0; i < 4; ++i) {
            if (L_Players[i] == NULL)
                continue;
            abs_x = abs(L_Players[i]->x - B->x);
            abs_y = abs(L_Players[i]->y - B->y);
            distance = round(sqrt(abs_x * abs_x + abs_y * abs_y));
            if (distance <= 3 && (B->x == L_Players[i]->x || B->y == L_Players[i]->y)) {
                //printf("Founde\n");
                if (B->x == L_Players[i]->x) {
                    for (int j = B->y > L_Players[i]->y ? L_Players[i]->y : B->y;
                         j < B->y > L_Players[i]->y ? B->y : L_Players[i]->y; ++j) {
                        if (map[j][B->x] == '#')
                        {
                            index=-1;
                            break;
                        }
                    }
                    index=i;
                    break;
                } else if (B->y == L_Players[i]->y) {
                    for (int j = B->x > L_Players[i]->x ? L_Players[i]->x : B->x;
                         j < B->x > L_Players[i]->x ? B->x : L_Players[i]->x; ++j) {
                        if (map[B->y][j] == '#') {
                            index=-1;
                            break;
                        }

                    }
                    index = i;
                    break;
                } else
                    continue;

            }

        }
        if (index == -1) {
            //pthread_create(&beast, NULL, (void *) move_beast, B);
            move_beast(B);
            sem_post(&sem);
            //pthread_join(beast, NULL);
            //pthread_exit(NULL);
            continue;
        } else {
            if (L_Players[index]->x == B->x && L_Players[index]->y < B->y&&map[B->y-1][B->x]!='#') {
                map[B->y][B->x] = ' ';
                B->y--;
                B->way = 'w';
                sem_post(&sem);
                continue;
            } else if (L_Players[index]->x == B->x && L_Players[index]->y > B->y&&map[B->y+1][B->x]!='#') {
                map[B->y][B->x] = ' ';
                B->y++;
                B->way = 's';
                sem_post(&sem);
                continue;
            } else if (L_Players[index]->y == B->y && L_Players[index]->x > B->x&&map[B->y][B->x+1]!='#') {
                map[B->y][B->x] = ' ';
                B->x++;
                B->way = 'd';
                sem_post(&sem);
                continue;
            } else if (L_Players[index]->y == B->y && L_Players[index]->x < B->x&&map[B->y][B->x-1]!='#') {
                map[B->y][B->x] = ' ';
                B->x--;
                B->way = 'a';
                sem_post(&sem);
                continue;
            }
            move_beast(B);
            sem_post(&sem);

        }
        sem_post(&sem);

    }
}


void create_map() {
    map = malloc(sizeof(char *) * MAP_HEIGHT);
    if (map == NULL) {
        abort();
    }
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        map[i] = malloc(sizeof(char) * (MAP_WIDTH + 2));
        if (map[i] == NULL) {
            free(map);
            abort();
        }
    }
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH + 1; ++j) {
            *(*(map + i) + j) = map_backup[i][j];
        }
        *(*(map + i) + MAP_WIDTH + 1) = '\0';
    }
}


int main() {
//    initscr();
    sem_init(&sem,0,1);
    create_map();
    sem_init(&get_key_sema_main,0,1);


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    int n = 0;
    pthread_t tim;

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_create(&input, &tattr, input_key, NULL);
    pthread_create(&thread_id_player_listen, NULL, (void *) listening, NULL);


    initscr();
    start_color();
    noecho();
    menuwin = newwin((MAP_HEIGHT+2),(MAP_WIDTH+2),0,0);
    if(menuwin==NULL)
        return 2;

    score_menu = newwin(10,58,0,(MAP_WIDTH+2));
    if(score_menu==NULL)
        return 2;
    legend = newwin(10,58,10,(MAP_WIDTH+2));
    if(legend==NULL)
        return 2;
    command = newwin(3,58,20,(MAP_WIDTH+2));
    if(command==NULL)
        return 2;
    keypad(command,TRUE);
    wclear(menuwin);
    wclear(command);
    wclear(score_menu);
    wclear(legend);
    box(menuwin,0,0);
    box(legend,0,0);
    box(command,0,0);
    box(score_menu,0,0);
    sem_init(&timer_sema,0,1);
    pthread_create(&tim, NULL, (void *) timer, NULL);
//    for (int i = 0; i < 4; ++i) {
//        pthread_create(&thread_id_key[i], NULL, (void *) get_key, L_Players[i]);
//    }
//    for (int i = 0; i < BEASTS; ++i) {
//        pthread_create(&thread_id[i], NULL, (void *) ai_beast, L_Beasts[i]);
//    }
//    for (int i = 0; i < 4; ++i) {
//        pthread_create(&thread_id_player[i], NULL, (void *) send_maze, L_Players[i]);
//    }

    while (1) {
        if (EXIT == 1)
            break;

        for (int i = 0; i < BEASTS; ++i) {
            if(L_Beasts[i]==NULL)
                continue;
            sem_post(&L_Beasts[i]->move_beast_semaphore);
        }
        for (int i = 0; i < PLAYERS; ++i) {
            if(L_Players[i]==NULL)
                continue;
            sem_post(&L_Players[i]->get_key_sema);
        }
        //sem_post(&sem);
        update_map();




        //display_maze();



        sem_wait(&timer_sema);
        for (int i = 0; i < 4; ++i) {
            if(L_Players[i]==NULL)
                continue;
            sem_post(&L_Players[i]->send_maze_semaphore);
        }
        display_maze();
        wrefresh(legend);
        wrefresh(score_menu);
        wrefresh(command);
        werase(legend);
        box(legend,0,0);
        werase(score_menu);
        box(score_menu,0,0);
        werase(command);
        box(command,0,0);
        wmove(command,1, 1);
        //sem_post(&timer_sema);


        //pthread_join(tim, NULL);
    }
    if(Exit_4==1)
        pthread_detach(input);
    //pthread_join(input, NULL);
    pthread_join(thread_id_player_listen, NULL);
    for (int i = 0; i < DROPS; ++i) {
        delete_drops(L_Drops[i]);
    }
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        free(map[i]);
    }
    free(map);
    if (thread_id != NULL)
        free(thread_id);
    sem_destroy(&sem);
    sem_destroy(&timer_sema);
    sem_destroy(&get_key_sema_main);
    endwin();
    return 0;


}
