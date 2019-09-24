#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * 1) Any live cell with fewer than two live neighbours dies, 
 *      as if caused by underpopulation.
 *
 * 2) Any live cell with two or three live neighbours lives 
 *      on to the next generation.
 *
 * 3) Any live cell with more than three live neighbours dies, 
 *      as if by overpopulation.
 *
 * 4) Any dead cell with exactly three live neighbours becomes a live cell, 
 *      as if by reproduction.
 */

#define DEAD_CHAR '.'
#define ALIVE_CHAR 'o'

int x_size, y_size;

enum cell_state {
    dead = 0, 
    alive = 1,
};
struct cell {
    int id;
    int pos_x;
    int pos_y;
    enum cell_state state;
    enum cell_state next_state;
};

struct cell **cells;

void display()
{
    system("clear");
    for (int y = 0; y < y_size; y++) {
        for (int x = 0; x < x_size; x++) {
            switch (cells[x][y].state) {
                case dead: 
                    printf(" ");
                    break;
                case alive:
                    printf("o");
                    break;
                default:
                    printf("E");
            }
        }
        printf("\n");
    }
}

void init_cell(char *path)
{
    char tmp_state;
    FILE *file;
   
    file = fopen(path, "r");
    if (file == NULL) {
        printf("fopen returned %s\n", strerror(errno));
        exit(-1);
    }
           
    fscanf(file, "{%d,%d} ", &x_size, &y_size);
    cells = malloc(sizeof *cells * x_size);
    for (int x = 0; x < x_size; x++) {
        cells[x] = malloc(sizeof *cells[x] * y_size);
    }

    /* scanf for int separated by a blank */
    for (int y = 0; y < y_size; y++) {
        for (int x = 0; x < x_size; x++) {
            fscanf(file, "%c ", &tmp_state);
            if (tmp_state == DEAD_CHAR) {
                cells[x][y].state = dead;
            }
            else if (tmp_state == ALIVE_CHAR) {
                cells[x][y].state = alive;
            }
            else {
                fprintf(stderr, "Error init cell char");
                exit(0);
            }
            cells[x][y].pos_x = x;
            cells[x][y].pos_y = y;
        }
    }
    fclose(file);
}

void apply_all(void (*apply)(struct cell *curr_cell))
{
    for (int y = 0; y < y_size; y++) {
        for (int x = 0; x < x_size; x++) {
            apply(&(cells[x][y]));
        }
    }
}

void post_transition(struct cell *curr_cell)
{
    curr_cell->state = curr_cell->next_state;
}

int is_cell(int x, int y) {
    return (x >= 0) && (y >= 0) && (x < x_size) && (y < y_size);
}

int get_ngbh_nb(struct cell *curr_cell)
{
    int x, y;
    int ngbh_nb;
    ngbh_nb = 0;
    x = curr_cell->pos_x;
    y = curr_cell->pos_y;
    if (is_cell(x-1, y+1) && cells[x-1][y+1].state == alive) ngbh_nb++;
    if (is_cell(x-1, y) && cells[x-1][y].state == alive) ngbh_nb++;
    if (is_cell(x-1, y-1) && cells[x-1][y-1].state == alive) ngbh_nb++;
    if (is_cell(x, y-1) && cells[x][y-1].state == alive) ngbh_nb++;
    if (is_cell(x, y+1) && cells[x][y+1].state == alive) ngbh_nb++;
    if (is_cell(x+1, y+1) && cells[x+1][y+1].state == alive) ngbh_nb++;
    if (is_cell(x+1, y) && cells[x+1][y].state == alive) ngbh_nb++;
    if (is_cell(x+1, y-1) && cells[x+1][y-1].state == alive) ngbh_nb++;
    return ngbh_nb;
}

void pre_transition(struct cell *curr_cell)
{
    int ngbh_nb;
    ngbh_nb = get_ngbh_nb(curr_cell);
    switch(curr_cell->state) {
        case dead:
            /* Reproduction */
            if (ngbh_nb == 3) {
                curr_cell->next_state = alive;
            }
            break;
        case alive:
            /* Underpopulation */
            if (ngbh_nb < 2) {
                curr_cell->next_state = dead;
            }
            /* Stable */
            if (ngbh_nb == 2 || ngbh_nb == 3) {
                curr_cell->next_state = alive;
            }
            /* Overpopulation */
            if (ngbh_nb > 3) {
                curr_cell->next_state = dead;
            }
            break;
    }
}

int fsm()
{
    while (1) {
        apply_all(pre_transition);
        display();
        apply_all(post_transition);
        struct timespec sleep_values;
        sleep_values.tv_sec = 0;
        sleep_values.tv_nsec = 100000000L;
        nanosleep(&sleep_values, NULL);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage : cellular-automate pattern \n");
        exit(1);
    }
    init_cell(argv[1]);
    fsm();
}
