/*
 * To run:
 * ./gol file1.txt  0  # run with config file file1.txt, do not print board
 * ./gol file1.txt  1  # run with config file file1.txt, ascii animation
 * ./gol file1.txt  2  # run with config file file1.txt, ParaVis animation
 *
 */
#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "colors.h"

/****************** Definitions **********************/
/* Three possible modes in which the GOL simulation can run */
#define OUTPUT_NONE   (0)   // with no animation
#define OUTPUT_ASCII  (1)   // with ascii animation
#define OUTPUT_VISI   (2)   // with ParaVis animation

/* Used to slow down animation run modes: usleep(SLEEP_USECS);
 * Change this value to make the animation run faster or slower
 */
//#define SLEEP_USECS  (1000000)
#define SLEEP_USECS    (100000)

/* A global variable to keep track of the number of live cells in the
 * world (this is the ONLY global variable you may use in your program)
 */
static int total_live = 0;

/* This struct represents all the data you need to keep track of your GOL
 * simulation.  Rather than passing individual arguments into each function,
 * we'll pass in everything in just one of these structs.
 * this is passed to play_gol, the main gol playing loop
 *
 * NOTE: You will need to use the provided fields here, but you'll also
 *       need to add additional fields. (note the nice field comments!)
 * NOTE: DO NOT CHANGE THE NAME OF THIS STRUCT!!!!
 */
struct gol_data {

    // NOTE: DO NOT CHANGE the names of these 4 fields (but USE them)
    int rows;  // the row dimension
    int cols;  // the column dimension
    int iters; // number of iterations to run the gol simulation
    int output_mode; // set to:  OUTPUT_NONE, OUTPUT_ASCII, or OUTPUT_VISI

    int coords; 
    int *next_board; 
    int *curr_board;


    /* fields used by ParaVis library (when run in OUTPUT_VISI mode). */
    // NOTE: DO NOT CHANGE their definitions BUT USE these fields
    visi_handle handle;
    color3 *image_buff;
};


/****************** Function Prototypes **********************/

/* the main gol game playing loop (prototype must match this) */
void play_gol(struct gol_data *data);

/* init gol data from the input file and run mode cmdline args */
int init_game_data_from_args(struct gol_data *data, char **argv);

// A mostly implemented function, but a bit more for you to add.
/* print board to the terminal (for OUTPUT_ASCII mode) */
void print_board(struct gol_data *data, int round);

void get_cell_neighbors(struct gol_data *data);
int check_neighbors(struct gol_data *data, int rows, int cols);
void update_colors(struct gol_data* data);


/************ Definitions for using ParVisi library ***********/
/* initialization for the ParaVisi library (DO NOT MODIFY) */
int setup_animation(struct gol_data* data);
/* register animation with ParaVisi library (DO NOT MODIFY) */
int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data);
/* name for visi (you may change the string value if you'd like) */
static char visi_name[] = "GOL!";


int main(int argc, char **argv) {

    int ret;
    struct gol_data data;
    double secs;

    struct timeval start_time, stop_time;

    /* check number of command line arguments */
    if (argc < 3) {
        printf("usage: %s <infile.txt> <output_mode>[0|1|2]\n", argv[0]);
        printf("(0: no visualization, 1: ASCII, 2: ParaVisi)\n");
        exit(1);
    }

    /* Initialize game state (all fields in data) from information
     * read from input file */
    ret = init_game_data_from_args(&data, argv);
    if (ret != 0) {
        printf("Initialization error: file %s, mode %s\n", argv[1], argv[2]);
        exit(1);
    }

    /* initialize ParaVisi animation (if applicable) */
    if (data.output_mode == OUTPUT_VISI) {
        setup_animation(&data);
    }

    /* ASCII output: clear screen & print the initial board */
    if (data.output_mode == OUTPUT_ASCII) {
        if (system("clear")) { perror("clear"); exit(1); }
        print_board(&data, 0);
    }

    /* Invoke play_gol in different ways based on the run mode */
    if (data.output_mode == OUTPUT_NONE) {  // run with no animation
        ret = gettimeofday(&start_time, NULL);
        play_gol(&data);
        ret = gettimeofday(&stop_time, NULL);
    }
    else if (data.output_mode == OUTPUT_ASCII) { // run with ascii animation
        ret = gettimeofday(&start_time, NULL);
        play_gol(&data);
        ret = gettimeofday(&stop_time, NULL);

        // clear the previous print_board output from the terminal:
        // (NOTE: you can comment out this line while debugging)
        if (system("clear")) { perror("clear"); exit(1); }

        // NOTE: DO NOT modify this call to print_board at the end
        //       (it's to help us with grading your output)
        print_board(&data, data.iters);
    }
    else {  // OUTPUT_VISI: run with ParaVisi animation
            // tell ParaVisi that it should run play_gol
        connect_animation(play_gol, &data);
        // start ParaVisi animation
        run_animation(data.handle, data.iters);
        //play_gol(&data); ##Don't Need
    }

    // NOTE: you need to determine how and where to add timing code
    //       in your program to measure the total time to play the given
    //       number of rounds played.
    if (data.output_mode != OUTPUT_VISI) {
        secs = 0.0;

        double elapsed_sec = stop_time.tv_sec - start_time.tv_sec;
        double elapsed_usec = stop_time.tv_usec - start_time.tv_usec;
        secs = elapsed_sec + (elapsed_usec / 1000000);

        /* Print the total runtime, in seconds. */
        // NOTE: do not modify these calls to fprintf
        fprintf(stdout, "Total time: %0.3f seconds\n", secs);
        fprintf(stdout, "Number of live cells after %d rounds: %d\n\n",
                data.iters, total_live);
    }

    free(data.curr_board);
    data.curr_board = NULL;
    free(data.next_board);
    data.next_board = NULL;

    return 0;
}

//       As always, add your own additional helper function(s)
//       for implementing partial functionality of these big
//       parts of the application, apply good top-down design
//       in your solution, and use incremental implementation
//       and testing as you go.

/* initialize the gol game state from command line arguments
 *       argv[1]: name of file to read game config state from
 *       argv[2]: run mode value
 * data: pointer to gol_data struct to initialize
 * argv: command line args
 *       argv[1]: name of file to read game config state from
 *       argv[2]: run mode
 * returns: 0 on success, 1 on error
 */
int init_game_data_from_args(struct gol_data *data, char **argv) {
    int ret;
    FILE *infile;

    infile = fopen(argv[1], "r");
    if(!infile) {
        return 1;
    }

    int row;
    ret = fscanf(infile, "%d", &row);
    if(ret != 1) { 
     return -1;
    }
    data->rows = row;

    int col;
    ret = fscanf(infile, "%d", &col);
    if(ret != 1) { 
     return -1;
    }
    data->cols = col;

    int iters;
    ret = fscanf(infile, "%d", &iters);
    if(ret != 1) { 
     return -1;
    }
    data->iters = iters;

    int coords;
    ret = fscanf(infile, "%d", &coords);
    if(ret != 1) { 
     return -1;
    }
    data->coords = coords;

    // initialize current board
    data->curr_board = malloc(sizeof(int) * data->rows * data->cols);
    if (data->curr_board == NULL) {
        printf("ERROR: malloc failed.");
        exit(1);
    }
    for (int i =0; i < data->rows; i++) {
        for (int j = 0; j < data->cols; j++) {
            data->curr_board[i*data->cols + j] = 0;
        }
    }

    // initialize next board
    data->next_board = malloc(sizeof(int) * data->rows * data->cols);
    if (data->next_board == NULL) {
        printf("ERROR: malloc failed.");
        exit(1);
    }
    for (int i = 0; i < data->rows; i++) {
        for (int j = 0; j < data->cols; j++) {
            data->next_board[i*data->cols + j] = 0;
        }
    }

    // load in x y values
    for (int i = 0; i < data->coords; i++){
        int x, y;
        ret = fscanf(infile, "%d", &x);
        if(ret != 1) { 
            return -1;
        }
        ret = fscanf(infile, "%d", &y);
        if(ret != 1) { 
            return -1;
        }
        data->curr_board[x*data->cols + y] = 1;
    }
    data->output_mode = atoi(argv[2]);
    
    return 0;
}

/* the gol application main loop function:
 *  runs rounds of GOL,
 *    * updates program state for next round (world and total_live)
 *    * performs any animation step based on the output/run mode
 *
 *   data: pointer to a struct gol_data  initialized with
 *         all GOL game playing state
 */
void play_gol(struct gol_data *data) {

    //  at the end of each round of GOL, determine if there is an
    //  animation step to take based on the output_mode,
    //   if ascii animation:
    //     (a) call system("clear") to clear previous world state from terminal
    //     (b) call print_board function to print current world state
    //     (c) call usleep(SLEEP_USECS) to slow down the animation
    //   if ParaVis animation:
    //     (a) call your function to update the color3 buffer
    //     (b) call draw_ready(data->handle)
    //     (c) call usleep(SLEEP_USECS) to slow down the animation

    int round;
    for (int i = 0; i < data->iters; i++) {
        round = i; 
        if (data->output_mode == OUTPUT_NONE) {
            get_cell_neighbors(data);
            total_live = 0;
            for (int i = 0; i < data->rows; ++i) {
                for (int j = 0; j < data->cols; ++j) {
                    if (data->curr_board[i*data->cols + j] == 1) { 
                        total_live++; //count the total live cells for the round
                    }
                }
            }

        } else if (data->output_mode == OUTPUT_ASCII) {
            get_cell_neighbors(data);
            system("clear");
            print_board(data, round);
            usleep(SLEEP_USECS);

        } else if (data->output_mode == OUTPUT_VISI) {
            get_cell_neighbors(data);
            update_colors(data);
            draw_ready(data->handle);
            usleep(SLEEP_USECS);
        }
        for (int i = 0; i < data->rows; i++){
            for (int j = 0; j < data->cols; j++){
                data->curr_board[i*data->cols + j] = data->next_board[i*data->cols + j];
                //reset the board by copying the next board to the current board
            }
        }
    }
}

/* This function determines whether or not a cell lives/dies based on the rules of 
game of life, and it updates the board accordingly.

    data: pointer to a struct gol_data initialized with
    all GOL game playing state
*/
void get_cell_neighbors(struct gol_data *data) {
    for (int i = 0; i < data->rows; i++){
        for (int j = 0; j < data->cols; j++){
            int num_alive = check_neighbors(data, i, j);
                // if current cell is alive
            if (data->curr_board[i*data->cols + j] == 1) { 
                if (num_alive <= 1 || num_alive >= 4) {
                    data->next_board[i*data->cols + j] = 0;
                } else {
                    data->next_board[i*data->cols + j] = 1;
                }
                // if current cell is dead
            } else if (data->curr_board[i*data->cols + j] == 0) { 
                if (num_alive == 3) {
                    data->next_board[i*data->cols + j] = 1;
                } else {
                    data->next_board[i*data->cols + j] = 0;
                }
            }
        }
    }
}

/*
This function calculates the neighbors of a cell on the board
and counts how many are alive for that cell.

    data: pointer to a struct gol_data initialized with
    all GOL game playing state
    rows: the row index on the board for the cell
    cols: the column index on the board for the cell

    returns: the number of alive neighbor cells
*/
int check_neighbors(struct gol_data *data, int rows, int cols) {
    int num_alive = 0;
    int i, j;
    for (i = -1; i < 2; i++) {
        for (j = -1; j < 2; j++) {
            // skips over the cell itself
            if (!((i == 0) && (j == 0))) { 
                int x = (i + rows + data->rows) % (data->rows);
                int y = (j + cols + data->cols) % (data->cols);
                // checks if neighbor is alive
                if (data->curr_board[x*data->cols + y] == 1) {
                    num_alive++;
                }
            }
        }
    }
    return num_alive;
}

/* Print the board to the terminal.

     data: pointer to a struct gol_data initialized with
     all GOL game playing state
 *   round: the current round number
 */
void print_board(struct gol_data *data, int round) {
    int i, j;
    if (data->output_mode == OUTPUT_ASCII) {
        /* Print the round number. */
        fprintf(stderr, "Round: %d\n", round);  
        total_live = 0;
        for (i = 0; i < data->rows; ++i) {
            for (j = 0; j < data->cols; ++j) {
                 //if the current cell is alive
                if (data->curr_board[i*data->cols + j] == 1) { 
                    fprintf(stderr, " @");
                    total_live++;
                } else {  
                    fprintf(stderr, " .");
                }
            }
            fprintf(stderr, "\n");
        }
    }
    if (data->output_mode == OUTPUT_ASCII) {
        /* Print the total number of live cells. */
        fprintf(stderr, "Live cells: %d\n\n", total_live);
    }
}

/* This function uses the ParaVisi library and updates each pixel
    of the program screen (board) with a color (pink if alive,
    black if dead).

     data: pointer to a struct gol_data initialized with
     all GOL game playing state
*/
void update_colors(struct gol_data* data){
    int i, j, r, c, buff_i;
    r = data->rows;
    c = data->cols;
    for (i = 0; i < r; i++) {
        for (j = 0; j < c; j++) {
            buff_i = (r - (i+1))*c + j;
            if (data->curr_board[i*data->cols + j] == 1) {
                data->image_buff[buff_i] = c3_pink;
            } else if (data->curr_board[i*data->cols + j] == 0) {
                data->image_buff[buff_i] = c3_black;
            }
        }
    }
}

/**********************************************************/
/***** START: DO NOT MODIFY THIS CODE *****/
/* initialize ParaVisi animation */
int setup_animation(struct gol_data* data) {
    /* connect handle to the animation */
    int num_threads = 1;
    data->handle = init_pthread_animation(num_threads, data->rows,
            data->cols, visi_name);
    if (data->handle == NULL) {
        printf("ERROR init_pthread_animation\n");
        exit(1);
    }
    // get the animation buffer
    data->image_buff = get_animation_buffer(data->handle);
    if(data->image_buff == NULL) {
        printf("ERROR get_animation_buffer returned NULL\n");
        exit(1);
    }
    return 0;
}

/* sequential wrapper functions around ParaVis library functions */
void (*mainloop)(struct gol_data *data);

void* seq_do_something(void * args){
    mainloop((struct gol_data *)args);
    return 0;
}

int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data)
{
    pthread_t pid;

    mainloop = applfunc;
    if( pthread_create(&pid, NULL, seq_do_something, (void *)data) ) {
        printf("pthread_created failed\n");
        return 1;
    }
    return 0;
}
/***** END: DO NOT MODIFY THIS CODE *****/
/******************************************************/
