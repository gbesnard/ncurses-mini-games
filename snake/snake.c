#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <time.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define xSize 50
#define ySize 20

static int direction = 1;
static int b = 1;

struct termios oldtermios;

typedef struct point
{
    int y;
    int x;
}t_point;
pthread_mutex_t mtx;

/* game */
void temporisation(double n);
char** map_assignement();
void launch_game(double t0);
void print_frame(char** map);
void next_pos(int direction, t_point* current_pos);
int test_collision(t_point current_pos, t_point tail[], int current_tail_size);
void direction_change(int keyboard_input);
void read_inputs();
static void * printing_loop(void * foo);

/* raw mod */
int ttyraw(int fd);
int ttyreset(int fd);
void sigcatch(int sig);


/* GAME */
//---------------------------------------------------------------------
//---------------------------------------------------------------------
// TODO : Still a problem on fast keyboard input
void main()
{   
    double t0 = 200;
    launch_game(t0);
    
    if(ttyreset(0) < 0)
    {
        fprintf(stderr, "Cannot reset terminal!\n");
        exit(-1);
    }

    system("clear");

    exit(0);
}

void launch_game(double t0)
{
    /* initialization */
    double t = t0;
    srand((unsigned int)time(NULL));
    pthread_t print_thread;

    /* set raw mode on stdin. */
    if(ttyraw(0) < 0)
    {
        fprintf(stderr,"Can't go to raw mode.\n");
        exit(1);
    }

    pthread_create(&print_thread, NULL, printing_loop, &t);
    read_inputs();
    
    pthread_join(print_thread, NULL);      
}

void direction_change(int keyboard_input)
{
    pthread_mutex_lock(&mtx);
    /* test the change of direction if it's possible */
    if (keyboard_input == 65 && direction != 2) /* tr_x up */
        direction = 0; 
    else if (keyboard_input == 67 && direction != 3) /* tr_x right */
        direction = 1;
    else if (keyboard_input== 66 && direction != 0) /* tr_x down */
        direction = 2;
    else if (keyboard_input==68 && direction != 1 ) /* tr_x left */
        direction = 3;
    pthread_mutex_unlock(&mtx);
}

void read_inputs()
{
    int c;
    int i;

    while((i = read(0, &c, 1)) == 1)
    {
        /* quit if ctrl-c, q */
        if(c == 3 || c == 113) 
        {
            b = 0;
            break;
        }

        /* ASCII delete */
        if(c == 0177) 
        {
            b = 0;
            break;
        }
            

        if(c == 27)
        {
            if((i = read(0, &c, 1)) == 1)
            {
                if (c == 91)
                {
                    if((i = read(0, &c, 1)) == 1)
                    {
                        direction_change(c);
                    } 
                }
            }
        } 
    }
} 

static void * printing_loop(void * foo)
{
    double *t_temp = foo;
    double t = *t_temp;
    int bonus_pop = 0;
    char** map = (char**)map_assignement();
    
    /* distance to the wall */
    int r_y = (rand()% (xSize-15)) + 10; 
    int r_x = (rand()% (ySize-15)) + 10;
    t_point current_pos = {r_x,r_y};

    t_point tail[xSize*ySize];
    int current_tail_size = 0;
    int tail_size = 20;

    int stable_direction;
    tail[current_tail_size] = current_pos; 
    current_tail_size++;

    system("clear");

    while (b)
    {
        /* get the head's position */
        //next_pos(stable_direction, &current_pos); 
        next_pos(direction, &current_pos);

        tail[current_tail_size] = current_pos;
        current_tail_size++;

        /* If bonus eaten : speed increased */
        if (map[current_pos.y][current_pos.x] == '#')
        {;
            tail_size++;
            bonus_pop = 0;
            t = t-2;
        }

        map[current_pos.y][current_pos.x] = 'o';

        int bonus_x = (rand()% (xSize-1));
        int bonus_y = (rand()% (ySize-1));
        if (bonus_pop == 0
        && map[bonus_y][bonus_x] != 'o'
        && map[bonus_y][bonus_x] != '*'
        && map[bonus_y][bonus_x] != '#')
        {
            map[bonus_y][bonus_x] = '#';
            bonus_pop = 1;
        }

        /* cut the tail */
        if (current_tail_size > tail_size)
        {
            map[tail[1].y][tail[1].x] =' ';
            int i;
            for (i=1; i < current_tail_size; i++)
            {
                tail[i-1] = tail[i];
            }
        current_tail_size--;
        }

        print_frame(map);

        /* Exit loop if collision */
        b = test_collision(current_pos, tail, current_tail_size); 
        
        //if (stable_direction == 0 || stable_direction == 2)
        if (direction == 0 || direction == 2)
        {
            temporisation(t*1.3);
        }
        else
        {
            temporisation(t);
        }
    }
    free(map);
}

int test_collision(t_point current_pos, t_point tail[], int current_tail_size)
{
    int i = 1;
    t_point p_test;

    /* doesn't consider the last position because it's the head */
    for (i = 1; i < current_tail_size-1; i++)
    {
        p_test = tail[i];
        if (p_test.x == current_pos.x && p_test.y == current_pos.y)
        {
            return 0;
        }
    }
    if((current_pos.x == 0)
        || (current_pos.y == 0)
        || (current_pos.x == xSize-1)
        || (current_pos.y == ySize-1))
            return 0;

    return 1;
}

char** map_assignement()
{   
    int x;
    int y;
    int range_size_x = (xSize+1)*sizeof(char);
    int range_size_y = (ySize+1)*sizeof(char);
    char** map = (char**)malloc(range_size_x*range_size_y);
    for (y=0; y < ySize; y++)
    {
        map[y] = (char*)malloc(range_size_x);
        for (x=0; x<xSize; x++)
        {
            if ((x == 0) || (y == 0) || (x == xSize-1) || (y == ySize-1)) map[y][x] = '*';
            else map[y][x] = ' ';
        }
        map[y][x] = '\0';
    }
    return map;
}

void next_pos(int direction, t_point* current_pos)
{
    pthread_mutex_lock(&mtx);
    if (direction == 0) current_pos->y--;
    else if (direction == 1) current_pos->x++;
    else if (direction == 2) current_pos->y++;
    else if (direction == 3) current_pos->x--;
    pthread_mutex_unlock(&mtx);
}

void print_frame(char** map)
{
    int y;
    system("clear");
    for (y=0;y<ySize;y++){
        printf("%s\n\r",map[y]);
    }
}

void temporisation(double n)
{
    clock_t start, stop ;
    n = n*CLOCKS_PER_SEC/1000 ; 
    start = clock() ;         
    stop = start + n ;      
    while(clock() < stop)
    {
    } 
}

/* RAW MOD */
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int ttyraw(int fd)
{
    /* Set terminal mode as follows:
       Noncanonical mode - turn off ICANON.
       Turn off signal-generation (ISIG)
        including BREAK character (BRKINT).
       Turn off any possible preprocessing of input (IEXTEN).
       Turn ECHO mode off.
       Disable CR-to-NL mapping on input.
       Disable input parity detection (INPCK).
       Disable stripping of eighth bit on input (ISTRIP).
       Disable flow control (IXON).
       Use eight bit characters (CS8).
       Disable parity checking (PARENB).
       Disable any implementation-dependent output processing (OPOST).
       One byte at a time input (MIN=1, TIME=0).
    */
    struct termios newtermios;
    if(tcgetattr(fd, &oldtermios) < 0)
        return(-1);
    newtermios = oldtermios;

    newtermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* OK, why IEXTEN? If IEXTEN is on, the DISCARD character
       is recognized and is not passed to the process. This 
       character causes output to be suspended until another
       DISCARD is received. The DSUSP character for job control,
       the LNEXT character that removes any special meaning of
       the following character, the REPRINT character, and some
       others are also in this categor_x.
    */

    newtermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* If an input character arrives with the wrong parity, then INPCK
       is checked. If this flag is set, then IGNPAR is checked
       to see if input bytes with parity errors should be ignored.
       If it shouldn't be ignored, then PARMRK determines what
       character sequence the process will actually see.
       
       When we turn off IXON, the start and stop characters can be read.
    */

    newtermios.c_cflag &= ~(CSIZE | PARENB);
    /* CSIZE is a mask that determines the number of bits per byte.
       PARENB enables parity checking on input and parity generation
       on output.
    */

    newtermios.c_cflag |= CS8;
    /* Set 8 bits per character. */

    newtermios.c_oflag &= ~(OPOST);
    /* This includes things like expanding tabs to spaces. */

    newtermios.c_cc[VMIN] = 1;
    newtermios.c_cc[VTIME] = 0;

    /* You tell me why TCSAFLUSH. */
    if(tcsetattr(fd, TCSAFLUSH, &newtermios) < 0)
        return(-1);
    return(0);
}
    
     
int ttyreset(int fd)
{
    if(tcsetattr(fd, TCSAFLUSH, &oldtermios) < 0)
        return(-1);

    return(0);
}

void sigcatch(int sig)
{
    ttyreset(0);
    exit(0);
}
