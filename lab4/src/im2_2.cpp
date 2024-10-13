#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <sys/wait.h>
#include "libppm.h"  // Includes definition of `image_t`

using namespace std;
using namespace std::chrono;

int shmid1, shmid2;
int semid;

union semun
{
    int val;
    struct semid_ds* buf;
    ushort* array;
};

struct image_t* S1_smoothen(struct image_t* input_image);
struct image_t* S2_find_details(struct image_t* input_image, struct image_t* smoothened_image);
struct image_t* S3_sharpen(struct image_t* input_image, struct image_t* details_image);

// uint8_t shared_img[1000 * 1000 * 3];
// shared_img[i][j][k] = shared_img[i * width * 3 + j * 3 + k]

void setup_ipc(int width, int height)
{
    // allocate shared memory of size width * height * 3
    shmid1 = shmget(IPC_PRIVATE, width * height * 3, IPC_CREAT | 0666);
    if (shmid1 < 0)
    {
        perror("shmget");
        exit(1);
    }

    // allocate shared memory of size width * height * 3
    shmid2 = shmget(IPC_PRIVATE, width * height * 3, IPC_CREAT | 0666);
    if (shmid2 < 0)
    {
        perror("shmget");
        exit(1);
    }

    semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666);
    if (semid < 0)
    {
        perror("semget");
        exit(1);
    }

    union semun sem_union;
    sem_union.val = 0;
    for (int i = 0; i < 4; ++i)
    {
        semctl(semid, i, SETVAL, sem_union);
    }
}

void sem_signal(int sem_num)
{
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = 1; // Unlock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void sem_wait(int sem_num)
{
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -1; // Lock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void cleanup_image_t(struct image_t* image)
{
    int w = image->width;
    int h = image->height;
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            delete[] image->image_pixels[i][j];
        }
        delete[] image->image_pixels[i];
    }
    delete[] image->image_pixels;
    delete image;
}


long long generatehash(struct image_t* image){
    long long sum=0;

    for(int i=0;i<image->height;i++)
	{
		for(int j=0;j<image->width;j++)
		{
            for(int k=0;k<3;k++)
            {
                //cout<<image->image_pixels[i][j][k]<<" ";
                //sum++;
                sum+=image->image_pixels[i][j][k];
                //sum^=image->image_pixels[i][j][k];
            }
            //cout<<endl;
        }
    }
    return sum;
}
struct image_t* convert_arr_to_image_t(uint8_t* arr, int width, int height)
{
    struct image_t* img = new image_t;
    img->width = width;
    img->height = height;
    img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++)
    {
        img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++)
        {
            img->image_pixels[i][j] = new uint8_t[3];
            for (int k = 0; k < 3; k++)
            {
                img->image_pixels[i][j][k] = arr[i * width * 3 + j * 3 + k];
            }
        }
    }
    return img;
}

struct image_t* copy_image(struct image_t* original)
{
    struct image_t* img = new image_t;
    img->width = original->width;
    img->height = original->height;
    img->image_pixels = new uint8_t**[img->height];
    for (int i = 0; i < img->height; i++)
    {
        img->image_pixels[i] = new uint8_t*[img->width];
        for (int j = 0; j < img->width; j++)
        {
            img->image_pixels[i][j] = new uint8_t[3];
            for (int k = 0; k < 3; k++)
            {
                img->image_pixels[i][j][k] = original->image_pixels[i][j][k];
            }
        }
    }
    return img;
}

void run_parallel(struct image_t* input_image, const char* output_filename)
{
    // store dimensions
    int width = input_image->width;
    int height = input_image->height;
    setup_ipc(width, height);

    pid_t pid_s1 = fork();
    if (pid_s1 == 0) // S1 process
{
    // cout << "Entered S1 Process" << endl; // Debug statement
    uint8_t* shm_s1 = (uint8_t*)shmat(shmid1, NULL, 0);
    if (shm_s1 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }

    for (int i = 0; i < 1000; ++i)
    {
        struct image_t* smooth_img = S1_smoothen(input_image);
        // cout << "S1 processing iteration: " << i << endl; // Debug statement
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                for (int k = 0; k < 3; k++)
                    shm_s1[i * width * 3 + j * 3 + k] = smooth_img->image_pixels[i][j][k];
        
        cleanup_image_t(smooth_img);
        sem_signal(0);
        sem_wait(1);
    }

    // cout << "Exiting S1 Process" << endl; // Debug statement
    shmdt(shm_s1);
    exit(0);
}


    pid_t pid_s2 = fork();
    if (pid_s2 == 0) // S2 process
{
    // cout << "Entered S2 Process" << endl; // Debug statement
    uint8_t* shm_s1 = (uint8_t*)shmat(shmid1, NULL, 0);
    uint8_t* shm_s2 = (uint8_t*)shmat(shmid2, NULL, 0);
    if (shm_s1 == (uint8_t*)(-1) || shm_s2 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }

    for (int i = 0; i < 1000; ++i)
    {
        sem_wait(0); // wait for S1 to finish
        // cout << "S2 processing iteration: " << i << endl; // Debug statement
        struct image_t* smoothened_image = convert_arr_to_image_t(shm_s1, width, height);
        sem_signal(1);
        struct image_t* details_img = S2_find_details(input_image, smoothened_image);
        cleanup_image_t(smoothened_image);
        
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                for (int k = 0; k < 3; k++)
                    shm_s2[i * width * 3 + j * 3 + k] = details_img->image_pixels[i][j][k];
        
        cleanup_image_t(details_img);
        sem_signal(2);
        sem_wait(3);
    }

    // cout << "Exiting S2 Process" << endl; // Debug statement
    shmdt(shm_s1);
    shmdt(shm_s2);
    exit(0);
}


    pid_t pid_s3 = fork(); // S3 process
    if (pid_s3 == 0) // S3 process
{
    // cout << "Entered S3 Process" << endl; // Debug statement
    uint8_t* shm_s2 = (uint8_t*)shmat(shmid2, NULL, 0);
    if (shm_s2 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }

    for (int i = 0; i < 1000; ++i)
    {
        sem_wait(2); // Wait for S2 to finish
        // cout << "S3 processing iteration: " << i << endl; // Debug statement
        struct image_t* details_image = convert_arr_to_image_t(shm_s2, width, height);

        sem_signal(3);
        
        struct image_t* sharpened_image = S3_sharpen(input_image, details_image);
        cleanup_image_t(details_image);
        write2_ppm_file((char*)output_filename, sharpened_image);
        cleanup_image_t(sharpened_image);
    }

    // cout << "Exiting S3 Process" << endl; // Debug statement
    shmdt(shm_s2);
    exit(0);
}


    wait(NULL);
    wait(NULL);
    wait(NULL);
    cleanup_image_t(input_image);

    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, NULL);
}

int main(int argc, char** argv)
{

    if (argc != 3)
    {
        cout << "Usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n";
        exit(0);
    }
        // Start measuring time using chrono
    auto start = high_resolution_clock::now();

    struct image_t* input_image = read_ppm_file(argv[1]);

    run_parallel(input_image, argv[2]);
    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end-start).count();
    std::cout << "Processing time: " << elapsed << " seconds" << std::endl;



    return 0;
}

// Function definitions
struct image_t* S1_smoothen(struct image_t* input_image)
{
    int width = input_image->width;
    int height = input_image->height;

    // Allocate new image
    struct image_t* smooth_img = new image_t;
    smooth_img->width = width;
    smooth_img->height = height;

    // Allocate 3D array for the image pixels
    smooth_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++)
    {
        smooth_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++)
        {
            smooth_img->image_pixels[i][j] = new uint8_t[3];

            // Copy original image pixels to smooth_img
            for (int k = 0; k < 3; k++)
            {
                smooth_img->image_pixels[i][j][k] = input_image->image_pixels[i][j][k];
            }
        }
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                for (int di = -1; di <= 1; di++)
                {
                    for (int dj = -1; dj <= 1; dj++)
                    {
                        if (i + di >= 0 && i + di < height && j + dj >= 0 && j + dj < width)
                        {
                            smooth_img->image_pixels[i][j][k] += input_image->image_pixels[i + di][j + dj][k];
                        }
                    }
                }
                smooth_img->image_pixels[i][j][k] /= 9;
            }
        }
    }
    return smooth_img;
}

struct image_t* S2_find_details(struct image_t* input_image, struct image_t* smoothened_image)
{
    int width = input_image->width;
    int height = input_image->height;

    struct image_t* details_img = new image_t;
    details_img->width = width;
    details_img->height = height;

    details_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++)
    {
        details_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++)
        {
            details_img->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++)
            {
                details_img->image_pixels[i][j][k] =
                    abs(input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k]);
            }
        }
    }
    return details_img;
}

struct image_t* S3_sharpen(struct image_t* input_image, struct image_t* details_image)
{
    int width = input_image->width;
    int height = input_image->height;

    struct image_t* sharpened_image = new image_t;
    sharpened_image->width = width;
    sharpened_image->height = height;

    sharpened_image->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++)
    {
        sharpened_image->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++)
        {
            sharpened_image->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++)
            {
                sharpened_image->image_pixels[i][j][k] = min(
                    255, max(0, (int)(input_image->image_pixels[i][j][k] + details_image->image_pixels[i][j][k])));
            }
        }
    }
    return sharpened_image;
}
