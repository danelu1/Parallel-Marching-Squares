// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "thread.h"

#define CONTOUR_CONFIG_COUNT 16
#define FILENAME_MAX_SIZE 50
#define STEP 8
#define SIGMA 200
#define RESCALE_X 2048
#define RESCALE_Y 2048

#define CLAMP(v, min, max) \
    if (v < min)           \
    {                      \
        v = min;           \
    }                      \
    else if (v > max)      \
    {                      \
        v = max;           \
    }

// Functie ce calculeazan minimul a doua numere.
int min(int a, int b)
{
    if (a < b)
    {
        return a;
    }

    return b;
}

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
ppm_image **init_contour_map()
{
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++)
    {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        map[i] = read_ppm(filename);
    }

    return map;
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y)
{
    for (int i = 0; i < contour->x; i++)
    {
        for (int j = 0; j < contour->y; j++)
        {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x)
{
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++)
    {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++)
    {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}

/*
    Functie folosita de fiecare thread in parte.
*/
void *thread_function(void *arg)
{
    struct thread_characteristics *threads = (struct thread_characteristics *)arg;

    pthread_barrier_t *barrier = threads->barrier;

    int threads_number = threads->P;
    int ID = threads->id;
    unsigned char sigma = threads->sigma;

    uint8_t sample[3];

    // Verific dimensiunile imaginii. Daca sunt mai mici sau egale cu 2048, nu e nevoie de redimensionare.
    // In caz contrar, redimensionez imaginea prin paralelizarea functiei "rescale_image" si prin utilizarea
    // unei bariere ca mecanism de sincronizare(vrem imaginea redimensionata pana sa continuam).
    if (threads->image->x <= 2048 && threads->image->y <= 2048)
    {
        threads->new_image = threads->image;
    }
    else
    {
        // Formule luate din laborator, folosite pentru paralelizare.
        int start = ID * (double)threads->new_image->x / threads_number;
        int end = min((ID + 1) * (double)threads->new_image->x / threads_number, threads->new_image->x);

        for (int i = start; i < end; i++)
        {
            for (int j = 0; j < threads->new_image->y; j++)
            {
                float u = (float)i / (float)(threads->new_image->x - 1);
                float v = (float)j / (float)(threads->new_image->y - 1);

                sample_bicubic(threads->image, u, v, sample);

                threads->new_image->data[i * threads->new_image->y + j].red = sample[0];
                threads->new_image->data[i * threads->new_image->y + j].green = sample[1];
                threads->new_image->data[i * threads->new_image->y + j].blue = sample[2];
            }
        }

        pthread_barrier_wait(barrier);
    }

    // Calculam dimensiunile grid-ului.
    int p = threads->new_image->x / threads->step_x;
    int q = threads->new_image->y / threads->step_y;

    int start = ID * (double)p / threads_number;
    int end = min((ID + 1) * (double)p / threads_number, p);

    // Paralelizarea functiei "sample_grid".
    for (int i = start; i < end; i++)
    {
        for (int j = 0; j < q; j++)
        {
            ppm_pixel curr_pixel = threads->new_image->data[i * threads->step_x * threads->new_image->y + j * threads->step_y];
            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > sigma)
            {
                threads->grid[i][j] = 0;
            }
            else
            {
                threads->grid[i][j] = 1;
            }
        }
    }

    for (int i = start; i < end; i++)
    {
        ppm_pixel curr_pixel = threads->new_image->data[i * threads->step_x * threads->new_image->y + threads->new_image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma)
        {
            threads->grid[i][q] = 0;
        }
        else
        {
            threads->grid[i][q] = 1;
        }
    }

    start = ID * (double)q / threads_number;
    end = min((ID + 1) * (double)q / threads_number, q);

    for (int j = start; j < end; j++)
    {
        ppm_pixel curr_pixel = threads->new_image->data[(threads->new_image->x - 1) * threads->new_image->y + j * threads->step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma)
        {
            threads->grid[p][j] = 0;
        }
        else
        {
            threads->grid[p][j] = 1;
        }
    }

    start = ID * (double)p / threads_number;
    end = min((ID + 1) * (double)p / threads_number, p);

    // Paralelizarea functiei "march".
    for (int i = start; i < end; i++)
    {
        for (int j = 0; j < q; j++)
        {
            unsigned char k = 8 * threads->grid[i][j] + 4 * threads->grid[i][j + 1] + 2 * threads->grid[i + 1][j + 1] + 1 * threads->grid[i + 1][j];
            update_image(threads->new_image, threads->map[k], i * threads->step_x, j * threads->step_y);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    // Variabila folosita pentru functiile specifice thread-urilor.
    int r;

    // Fisierul de "input" primit in linia de comanda.
    char *input_file = argv[1];

    // Fisierul de "output" primit in linia de comanda.
    char *output_file = argv[2];

    // Numarul de thread-uri(tot din linia de comanda).
    int threads_number = atoi(argv[3]);

    // Thread-urile ce vor rula functia "thread_function".
    pthread_t threads[threads_number];

    // Bariera folosita pentru sincronizare.
    pthread_barrier_t barrier;

    // Initializez bariera.
    r = pthread_barrier_init(&barrier, NULL, threads_number);

    // Vectorul de caracteristici ale thread-urilor.
    struct thread_characteristics threads_chs[threads_number];

    ppm_image *image = read_ppm(input_file);
    int step_x = STEP;
    int step_y = STEP;

    // Variabila in care retin fiecare contor.
    ppm_image **contour_map = init_contour_map();

    // Aloc memorie pentru grid.
    unsigned char **grid = (unsigned char **)malloc((RESCALE_X / step_x + 1) * sizeof(unsigned char *));

    for (int i = 0; i < RESCALE_X / step_x + 1; i++)
    {
        grid[i] = (unsigned char *)malloc((RESCALE_Y / step_y + 1) * sizeof(unsigned char));
    }

    // Aloc memorie pentru rezultat.
    ppm_image *new_image = (ppm_image *)malloc(sizeof(ppm_image));

    // Initializez fiecare camp din structura pentru fiecare thread.
    for (int i = 0; i < threads_number; i++)
    {
        threads_chs[i].id = i;
        threads_chs[i].map = contour_map;
        threads_chs[i].P = threads_number;
        threads_chs[i].sigma = SIGMA;
        threads_chs[i].image = image;
        threads_chs[i].barrier = &barrier;
        threads_chs[i].step_x = step_x;
        threads_chs[i].step_y = step_y;
        threads_chs[i].grid = grid;
        strcpy(threads_chs[i].filename, output_file);
        threads_chs[i].new_image = new_image;
        threads_chs[i].new_image->x = RESCALE_X;
        threads_chs[i].new_image->y = RESCALE_Y;
        threads_chs[i].new_image->data = (ppm_pixel *)malloc(threads_chs[i].new_image->x * threads_chs[i].new_image->y * sizeof(ppm_pixel));
    }

    // Pornesc thread-urile.
    for (int i = 0; i < threads_number; i++)
    {
        r = pthread_create(&threads[i], NULL, thread_function, &threads_chs[i]);
        if (r)
        {
            printf("Error on thread %d\n", i);
            exit(-1);
        }
    }

    // Le dau "join".
    for (int i = 0; i < threads_number; i++)
    {
        r = pthread_join(threads[i], NULL);
        if (r)
        {
            printf("Error on thread %d\n", i);
            exit(-1);
        }
    }

    // Scriu output-ul in fisierul citit in linia de comanda.
    write_ppm(threads_chs[0].new_image, output_file);

    // Distrug bariera.
    r = pthread_barrier_destroy(&barrier);

    // Eliberez memoria alocata.
    free_resources(threads_chs[0].new_image, contour_map, grid, step_x);

    return 0;
}
