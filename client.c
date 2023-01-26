//
// Created by mario on 28.12.2021.
//
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
#include <sys/types.h>
#include <pthread.h>
#include <wchar.h>
#include <semaphore.h>
#include <locale.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8595
int MAP_WIDTH= 0;
int MAP_HEIGHT= 0;
#define SERVER_IP "127.0.0.1"
char **map=NULL;
int EXIT=0;
int x=0;
int y=0;
int sock=0;
int key = 5;
WINDOW * menuwin;
WINDOW * score_menu;
WINDOW * legend;
WINDOW * command;
int time_game=0;
int campsite_x=-1;
int campsite_y=-1;
sem_t sem;
int brought_money=0;
int carried_money=0;
int symbol_p='0';
void display_maze(){
    //sem_wait(&sem);
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
    wattron(menuwin,COLOR_PAIR(3));
    mvwprintw(menuwin,y+1,x+1,"%c",symbol_p);
    wattroff(menuwin,COLOR_PAIR(3));
    mvwprintw(score_menu,1,1,"Server Info IP: %s",SERVER_IP);
    if(campsite_x==-1||campsite_y==-1)
        mvwprintw(score_menu,2,2,"Campsite X/Y: ?/?");
    else
        mvwprintw(score_menu,2,2,"Campsite X/Y: %d/%d",campsite_x,campsite_y);
    mvwprintw(score_menu,3,2,"Tick: %d",time_game);
    mvwprintw(score_menu,4,2,"Player: %d",symbol_p-'0');
    mvwprintw(score_menu,5,3,"Position: %d %d",x,y);
    mvwprintw(score_menu,7,3,"Carried money: %d",carried_money);
    mvwprintw(score_menu,8,3,"Brought money: %d",brought_money);
    mvwprintw(legend,1,1,"Type: q to quit");
    mvwprintw(legend,2,1,"Type: keypad to move");
    mvwprintw(legend,3,1,"* - beast");
    mvwprintw(legend,4,1,"1 2 3 4 - players");
    mvwprintw(legend,5,1,"t - 10 coins");
    mvwprintw(legend,6,1,"C - 1 coin");
    mvwprintw(legend,7,1,"T - 50 coins");
    //wrefresh(menuwin);
    //sem_post(&sem);
}
void *update_map()
{
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if(map[i][j]=='A') {
                campsite_y = i;
                campsite_x = j;
            }
            if(map[i][j]=='#')
                map[i][j]= '=';
            if(map[i][j]=='*')
                map[i][j]= ' ';
            if(map[i][j]=='t')
                map[i][j]= ' ';
            if(map[i][j]=='T')
                map[i][j]= ' ';
            if(map[i][j]=='C')
                map[i][j]= ' ';
            if(isdigit(map[i][j]))
                map[i][j]= ' ';
        }
    }
}
void *key_input();
void *read_map() {
    int h=0;

    char map_5X5[5][5];
    while (1)
    {
        
        if(EXIT==1)
        {
            sem_post(&sem);
            pthread_exit(NULL);
        }

		update_map();
		sem_wait(&sem);
//        pthread_mutex_lock(&lock);
        h = read(sock, &x, (sizeof(int)));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            sem_post(&sem);
            pthread_exit(NULL);
        }
        
        h = read(sock, &y, (sizeof(int)));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            pthread_exit(NULL);
        }
        h = read(sock, &carried_money, (sizeof(int)));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            sem_post(&sem);
            pthread_exit(NULL);
        }
        h = read(sock, &brought_money, (sizeof(int)));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            sem_post(&sem);
            pthread_exit(NULL);
        }
        h = read(sock, &symbol_p, (sizeof(char )));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            sem_post(&sem);
            pthread_exit(NULL);
        }
        h = read(sock, &time_game, (sizeof(int)));
        if(h==0) {
//            pthread_mutex_unlock(&lock);
            EXIT=1;
            sem_post(&sem);
            pthread_exit(NULL);
        }
        for (int i = 0; i < 5; ++i) {
            h = read(sock, map_5X5[i], 5);
            if(h==0)
            {
//                pthread_mutex_unlock(&lock);
                EXIT=1;
                sem_post(&sem);
                pthread_exit(NULL);
            }
        }
        for (int i = -2,g=0; i < 3; ++i,g++) {
            for (int j = -2,z=0; j < 3; ++j,z++) {
                if(map_5X5[g][z]=='\0')
                    continue;
                map[y+i][x+j]=map_5X5[g][z];
            }
        }

//        printf("Odpowiedz: %d\n",h);
//        clear();
//        refresh();
        
        display_maze();
        sem_post(&sem);
        wrefresh(menuwin);


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


//        pthread_mutex_unlock(&lock);
//        sleep(1);
    }
}


void *key_input()
{
    int tmp=time_game;
    while(1) {
        key=5;
        flushinp();
        //sem_wait(&sem);
        if (time_game==tmp)
        {
            //sem_post(&sem);
            continue;
        }

        tmp=time_game;
        //sem_post(&sem);
        key = wgetch(command);
        


//        printw("%d\n",key);
        if (key == KEY_DOWN)
        {
            key='s';
            //sem_wait(&sem);
            //mvwprintw(command,1,1,"You pressed down");
            //sem_post(&sem);
            send(sock, &key, sizeof(int), MSG_NOSIGNAL);
        }

        else if (key == KEY_RIGHT)
        {
            key='d';
            //sem_wait(&sem);
            //mvwprintw(command,1,1,"You pressed right");
            //sem_post(&sem);
            send(sock, &key, sizeof(int), MSG_NOSIGNAL);
        }

        else if (key == KEY_UP)
        {
            key='w';
            //sem_wait(&sem);
            //mvwprintw(command,1,1,"You pressed up");
            //sem_post(&sem);
            send(sock, &key, sizeof(int), MSG_NOSIGNAL);
        }

        else if (key == KEY_LEFT)
        {
            key='a';
            //sem_wait(&sem);
            //mvwprintw(command,1,1,"You pressed left");
            //sem_post(&sem);
            send(sock, &key, sizeof(int), MSG_NOSIGNAL);
        }
        else if(key=='q')
        {
			//sem_wait(&sem);
            //mvwprintw(command,1,1,"You pressed q");
            close(sock);
            //sem_post(&sem);
            EXIT=1;
            pthread_exit(NULL);
        }
        else
            //mvwprintw(command,1,1,"You pressed %c",key);
        //sem_post(&sem);
        usleep(700000);
        //sleep(1);
        

    }
}
int main()
{
    sem_init(&sem,0,1);
    setlocale(LC_ALL, "en_US.UTF-8");


//    refresh();
//    wrefresh(menuwin);
    int pid=getpid();

    int valread;
    struct sockaddr_in serv_addr;
    if((sock= socket(AF_INET,SOCK_STREAM,0))<0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port= htons(PORT);
    if (inet_pton(AF_INET,SERVER_IP,&serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sock,(struct sockaddr*)&serv_addr,sizeof (serv_addr))<0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    int h = read(sock, &MAP_HEIGHT, (sizeof(int)));
    if(h==0) {
        return 0;
    }
    h = read(sock, &MAP_WIDTH, (sizeof(int)));
    if(h==0) {
        return 0;
    }
    initscr();
    start_color();
    noecho();
    menuwin = newwin((MAP_HEIGHT+2),(MAP_WIDTH+2),0,0);
    if(menuwin==NULL)
        return 2;

    score_menu = newwin(10,40,0,(MAP_WIDTH+2));
    if(score_menu==NULL)
        return 2;
    legend = newwin(10,40,10,(MAP_WIDTH+2));
    if(legend==NULL)
        return 2;
    command = newwin(3,40,20,(MAP_WIDTH+2));
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
    map= malloc(sizeof(char*)*MAP_HEIGHT);
    if(map==NULL)
        return 1;
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        map[i]= malloc(sizeof(char)*(MAP_WIDTH+2));
        if(map[i]==NULL)
        {
            free(map);
            return 1;
        }
        //for (int j = 0; j < MAP_WIDTH; ++j) {
        //   map[i][j]=' ';
        //}
        //map[i][MAP_WIDTH]='\n';
        //map[i][MAP_WIDTH+1]='\0';
    }

    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_t thread_id,thread_key;
    pthread_create(&thread_id,NULL,(void*)read_map,NULL);
    pthread_create(&thread_key,&tattr,(void*)key_input,NULL);
    pthread_join(thread_id,NULL);
    if(EXIT==0)
        pthread_detach(thread_key);
    //delwin(menuwin);
    //delwin(legend);
    //delwin(score_menu);
    //delwin(command);
    endwin();


    return 0;
}

