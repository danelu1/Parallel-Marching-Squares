#include <pthread.h>

struct thread_characteristics
{
    int id;
    int step_x;
    int step_y;
    unsigned char sigma;
    int P;
    ppm_image *image;
    ppm_image *new_image;
    ppm_image **map;
    unsigned char **grid;
    char filename[50];
    pthread_barrier_t *barrier;
};