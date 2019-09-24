#include <ncurses.h>          /* ncurses.h includes stdio.h */
#include <string.h>
#include <time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "pthread_sleep.h"

static int row, col;
static pthread_mutex_t my_mutex;

double rand_interval(double min, double max)
{
        double max_t, min_t;
        if (min < max)
        {
                max_t = max;
                min_t = min;        
        }
        else
        {
            max_t = min;
            min_t = max;
        }

        double r;
        const int range = 1 + max_t - min_t;
        const int buckets = RAND_MAX / range;
        const int limit = buckets * range;

        /* create equal size buckets all in a row, then fire randomly towards
           the buckets until you land in one of them. All buckets are equally
           likely. If you land off the end of the line of buckets, try again  */
        do
        {
            r = rand();
        } while (r >= limit);

        return min_t + (r / buckets);
}

static void* thread_drawing(void *foo)
{
        int my_col = *((int*)foo); /* col to print on for this thread */
        double temp_t = rand_interval(0.01, 1); /* define the speed of print */
        double length_t = rand_interval(1, row); /* length of the code line */
        int i = 0; /* to know if you reached the last row */
        char rdm_char;

        while (1)
        {
                rdm_char = rand_interval(32, 126); /* pick a random ASCII char */
                pthread_mutex_lock(&my_mutex);
                
                if (i < row)     
                {
                    mvprintw(i, my_col, "%c", rdm_char); /* print code char */
                }          
                mvprintw(((double)i) - length_t, my_col, "%c",' '); /* delete code char */
                refresh();
                pthread_mutex_unlock(&my_mutex); /* unlock the access to the screen printing */

                pthread_sleep(temp_t); /* sleep after critical section */
                i = i + 1;
                if (((double)i) - length_t > row) /* if last row reached, and tail's deleted */
                {
                        i = 0;
                        temp_t = rand_interval(0.01, 1);
                        length_t = rand_interval(1, row);
                }
        }
        return NULL;
}

int main()
{
        initscr();
        if(has_colors() == FALSE) 
        {
                endwin();
                printf("Your terminal does not support color\n");
                exit(1);
        }

        getmaxyx(stdscr,row,col);       /* get the number of rows and columns */
        start_color();  /* start color */
        init_pair(1, COLOR_GREEN, COLOR_BLACK);

        attron(COLOR_PAIR(1)); /* will now print in green on black */

        pthread_mutex_init(&my_mutex, NULL);
        pthread_t drw_threads[col]; /* init and launch threads */
        int *thread_id = malloc(col * sizeof(int));
        for (int i = 0; i < col; i++)
        {
                thread_id[i] = i;
                pthread_create(&drw_threads[i], NULL, thread_drawing, &thread_id[i]);
        }

        char *rdm_char = malloc(1 * sizeof(int));
        char *tampon_col = calloc(col, sizeof(int)); /* memory cleared to 0 before allocation */

        /* end of sync tools */
        for (int i = 0; i < col; i++)
        {
            pthread_join(drw_threads[i], NULL);
        }
        pthread_mutex_destroy(&my_mutex);

        /* free memory */
        free (rdm_char);
        free (tampon_col);

        attroff(COLOR_PAIR(1)); /* stop green on black color */
        getch(); /* wait for an input before ending the window */
        endwin();

        return 0;
}
