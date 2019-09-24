#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ncurses.h> 
#include <pthread.h>

/*
    only consider moon gravity
    only vertical vectors
    no atmosphere
*/

#define MN_RADIUS 1737100.0 /* moon radius in meters */
#define GRAV_CST 1.6251     /* moon gravitational constant 
                               at 0 metres in m.s^-2 */     
#define RFRSH_RATE 0.5      /* refreshing rate in secondes */
#define STRT_ALT 10000.0    /* beginning altitude in meters */ 

#define FUEL_CPCTY     9000000.0 /* fuel mass in g */
#define LDER_MASS_EPTY 3920000.0 /* empty lander mass in g */

#define BSTR_CONSO_MAX 50000.0 /* fuel consummed in g.s^-1 */
#define BSTER_THRST_MAX 90000.0  /* max booster thrust in N */

static pthread_mutex_t input_mutex;
static int lander_thrust_prcnt; /* booster % activated */
static double current_altitude;
static double start_zoom_altitude;
static int is_tank_empty;
static int is_over;
static int row, col;

 
/*  get the gravity forces in fonction of 
    the altitude : not relevent at low altitude */ 
double compute_current_gravity(double current_altitude)
{
    return  GRAV_CST * 
        pow(MN_RADIUS / (MN_RADIUS + current_altitude), 2);
}

/*  check for booster consommation
    change the lander's mass
    change thruster acceleration using newton's second law :
    F = ma => a = F/m with F = Fmax_moteur * (booster % activated) 
    with F in Newton, m in kg and a in m.s^-2 */
void compute_thruster_acceleration(double *current_mass, double *booster_conso, double *thruster_acceleration)
{
    /* check for tank emptyness */
    if (!is_tank_empty)
    {
        *booster_conso = BSTR_CONSO_MAX * ((double)lander_thrust_prcnt / 100);

        /* substract the fuel needed for the burn */
        double temp_mass = *current_mass - *booster_conso;
        if (temp_mass >= LDER_MASS_EPTY)
        {
            if (lander_thrust_prcnt != 0)
            {
                *thruster_acceleration = -((BSTER_THRST_MAX * ((double)lander_thrust_prcnt / 100)) / 
                    (*current_mass / 1000));
            }
            else
            {
                *thruster_acceleration = 0;
            }
            *current_mass = temp_mass;
        }
        else
        {
            *current_mass = LDER_MASS_EPTY;
            *booster_conso = 0;
            *thruster_acceleration = 0;
            lander_thrust_prcnt = 0;
            is_tank_empty = 1;
        }
    }
}

/* exec by another thread */
static void* get_keyboard_input() 
{
    while (!is_over)
    {
        /* get user input */
        int user_input = getch();    

        pthread_mutex_lock(&input_mutex);
        if(user_input == KEY_UP && lander_thrust_prcnt < 100)
            lander_thrust_prcnt += 1;
        else if (user_input == KEY_DOWN && lander_thrust_prcnt > 0)
            lander_thrust_prcnt -= 1;
        else if (user_input == 'z')
            start_zoom_altitude = current_altitude; 
        pthread_mutex_unlock(&input_mutex);
    }
    return 0;
}

int main()
{   
    /* init */
    double vertical_velocity = 100;
    double seconds_elapsed = 0;
    current_altitude = STRT_ALT;
    start_zoom_altitude = STRT_ALT;
    double current_fuel_capacity = FUEL_CPCTY;
    double current_mass = current_fuel_capacity + LDER_MASS_EPTY;
    double current_acceleration = 0;
    double booster_conso = 0;
    double thruster_acceleration = 0;
    lander_thrust_prcnt = 0; /* thrust in m.s^2 */
    is_tank_empty = 0;
    is_over = 0;
    int user_input = -1;
    double current_gravity = compute_current_gravity(current_altitude);

    /* struct for nanosleep */
    struct timespec ts; 
    ts.tv_sec = (time_t) RFRSH_RATE;
    ts.tv_nsec = (long) ((RFRSH_RATE - ts.tv_sec) * 1e+9);
    
    /* init ncurses */
    initscr();
    getmaxyx(stdscr,row,col); /* get the number of rows and columns */
    nodelay(stdscr, TRUE); /* so getch function is not waiting for input */
    keypad(stdscr, TRUE); /* to be able to get keyboard input */
    noecho(); /* so keyboard input does'nt echo on the terminal */

    /* init input thread */
    pthread_t reading_thread; 
    pthread_mutex_init(&input_mutex, NULL);
    pthread_create(&reading_thread, NULL, get_keyboard_input, NULL);

    /* assuming 1 loop last 1 seconds */
    while (!is_over)
    {
        pthread_mutex_lock(&input_mutex);
        /* update gravity var */
        seconds_elapsed++;

        // Newton's law of motion 
        double temp_velocity = vertical_velocity;
        compute_thruster_acceleration(&current_mass, &booster_conso, &thruster_acceleration);
        current_fuel_capacity = current_mass - LDER_MASS_EPTY;

        vertical_velocity += (current_gravity + thruster_acceleration) * RFRSH_RATE;
        current_acceleration = (vertical_velocity - temp_velocity) / RFRSH_RATE; // TODO : To check

        current_altitude -= vertical_velocity * RFRSH_RATE;
        current_gravity = compute_current_gravity(current_altitude);

        /* print gravity var */
        mvprintw(1, 1, " || gravity : %f m.s^-2 \n", current_gravity);
        mvprintw(2, 1, " || booster_acceleration : %f m.s^-2 \n", thruster_acceleration);
        mvprintw(3, 1, " || global_acceleration : %f m.s^-2 \n", current_acceleration);

        mvprintw(5, 1, " || velocity %.1f m/s \n", vertical_velocity);
        mvprintw(6, 1, " || altitude : %.1f m \n", current_altitude);
        
        mvprintw(8, 1, " || mass : %1.f kg \n", current_mass / 1000.0);
        mvprintw(9, 1, " || fuel_capacity : %1.f kg \n", current_fuel_capacity / 1000.0);
        mvprintw(10, 1, " || booster_activation : %i %% \n", lander_thrust_prcnt);
        mvprintw(11, 1, " || booster_conso : %1.f g/s \n", booster_conso);

        mvprintw (14, 1, "\n");
        
        double ratio_to_print = current_altitude/start_zoom_altitude;
        double col_to_soustract = (col-11) * ratio_to_print;
        int col_to_print_lander = (int)((col-11) - col_to_soustract); 
        
        if (lander_thrust_prcnt == 0)
            mvprintw (14, col_to_print_lander, "<=|");
        else if (lander_thrust_prcnt < 10)
            mvprintw (14, col_to_print_lander, "<=|~");
        else if (lander_thrust_prcnt < 50)
            mvprintw (14, col_to_print_lander, "<=|{~");
        else
            mvprintw (14, col_to_print_lander, "<=|{{~");

        mvprintw(12, col-10, "  ######      ");
        mvprintw(13, col-10, " #      #     ");
        mvprintw(14, col-10, "#  MOON  #    ");
        mvprintw(15, col-10, " #      #     ");
        mvprintw(16, col-10, "  ######      ");

        pthread_mutex_unlock(&input_mutex);
        
        if (current_altitude <= 0)
        {
            is_over = 1; 
            ts.tv_sec = 10;
         
            /* test if landing successful */
            if (vertical_velocity < 1)
                printf("\n\nPerfect landing !\n\n");
            else if (vertical_velocity < 2)
                printf("\n\nSmooth landing !\n\n");
            else if (vertical_velocity < 5) 
                printf("\n\nLanded !\n\n");
            else if (vertical_velocity < 10)
                printf("\n\nBrutal Landing !\n\n");
            else
                printf("\n\nHuston, we've got a problem !\n\n");
        }
        nanosleep(&ts, NULL); 
    }

    pthread_mutex_destroy(&input_mutex);
    getch();
    endwin();
    
    return 0;
}
