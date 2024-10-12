
#include <bits/stdc++.h> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <sys/wait.h>  // Include this header for wait function
#include "libppm.h"  // Includes definition of `image_t`

using namespace std;
using namespace std::chrono;

int shmid1, shmid2;
int semid;

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

struct image_t* S1_smoothen(struct image_t *input_image);
struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image);
struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image);

void setup_ipc() {
    shmid1 = shmget(IPC_PRIVATE, sizeof(struct image_t), IPC_CREAT | 0666);
    if (shmid1 < 0) {
        perror("shmget");
        exit(1);
    }

    shmid2 = shmget(IPC_PRIVATE, sizeof(struct image_t), IPC_CREAT | 0666);
    if (shmid2 < 0) {
        perror("shmget");
        exit(1);
    }

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }

    union semun sem_union;
    sem_union.val = 0;
    for (int i = 0; i < 2; ++i) {
        semctl(semid, i, SETVAL, sem_union);
    }
}

void sem_signal(int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = 1;  // Unlock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void sem_wait(int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -1;  // Lock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void run_parallel(struct image_t* input_image, const char* output_filename) {
    setup_ipc();

    pid_t pid_s1 = fork();
    if (pid_s1 == 0) { // S1 process
        struct image_t *shm_s1 = (struct image_t*)shmat(shmid1, NULL, 0);
        if (shm_s1 == (struct image_t*)(-1)) {
            perror("shmat");
            exit(1);
        }
        for (int i = 0; i < 1000; ++i) { 
            struct image_t* smooth_img = S1_smoothen(input_image);
            memcpy(shm_s1, smooth_img, sizeof(struct image_t)); // Copy result to shared memory
            sem_signal(0);  // Signal S2
            delete smooth_img; // Free the temporary smoothened image
        }
        shmdt(shm_s1);
        exit(0);
    }

    pid_t pid_s2 = fork();
    if (pid_s2 == 0) { // S2 process
        struct image_t *shm_s1 = (struct image_t*)shmat(shmid1, NULL, 0);
        struct image_t *shm_s2 = (struct image_t*)shmat(shmid2, NULL, 0);
        if (shm_s1 == (struct image_t*)(-1) || shm_s2 == (struct image_t*)(-1)) {
            perror("shmat");
            exit(1);
        }
        for (int i = 0; i < 1000; ++i) { 
            sem_wait(0);  // Wait for S1 to finish
            struct image_t* details_img = S2_find_details(input_image, shm_s1);
            memcpy(shm_s2, details_img, sizeof(struct image_t)); // Copy result to shared memory
            sem_signal(1);  
            delete details_img; // Free the temporary details image
        }
        shmdt(shm_s1);
        shmdt(shm_s2);
        exit(0);
    }

    pid_t pid_s3 = fork(); // S3 process
    if (pid_s3 == 0) {
        struct image_t *shm_s2 = (struct image_t*)shmat(shmid2, NULL, 0);
        if (shm_s2 == (struct image_t*)(-1)) {
            perror("shmat");
            exit(1);
        }
        for (int i = 0; i < 1000; ++i) { 
            sem_wait(1); // Wait for S2 to finish
            struct image_t *sharpened_image = S3_sharpen(input_image, shm_s2);
            write2_ppm_file((char*)output_filename, sharpened_image); 
            delete sharpened_image; // Free the sharpened image
        }
        shmdt(shm_s2);
        exit(0);
    }

    wait(NULL);
    wait(NULL);
    wait(NULL);

    shmctl(shmid1, IPC_RMID, NULL); 
    shmctl(shmid2, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, NULL);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }

    struct image_t *input_image = read_ppm_file(argv[1]);

    run_parallel(input_image, argv[2]);

    return 0;
}

// Function definitions
struct image_t* S1_smoothen(struct image_t *input_image) {
    int width = input_image->width;
    int height = input_image->height;

    // Allocate new image
    struct image_t* smooth_img = new image_t;
    smooth_img->width = width;
    smooth_img->height = height;

    // Allocate 3D array for the image pixels
    smooth_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        smooth_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            smooth_img->image_pixels[i][j] = new uint8_t[3];

            // Copy original image pixels to smooth_img
            for (int k = 0; k < 3; k++) {
                smooth_img->image_pixels[i][j][k] = input_image->image_pixels[i][j][k];
            }
        }
    }

    for(int i=0; i<height; i++) {
        for(int j=0; j<width; j++) {
            if(i == 0 || j == 0 || i == height-1 || j == width-1){
                continue;
            }
            else {
                for(int k=0; k<3; k++) {
                    smooth_img->image_pixels[i][j][k] = (
                        input_image->image_pixels[i-1][j][k] +
                        input_image->image_pixels[i][j-1][k] +
                        input_image->image_pixels[i-1][j-1][k] +
                        input_image->image_pixels[i+1][j][k] +
                        input_image->image_pixels[i][j+1][k] +
                        input_image->image_pixels[i+1][j+1][k] +
                        input_image->image_pixels[i-1][j+1][k] +
                        input_image->image_pixels[i+1][j-1][k] +
                        input_image->image_pixels[i][j][k]) / 9;
                }
            }
        }
    }
    return smooth_img;
}

struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image) {
    int width = input_image->width;
    int height = input_image->height;

    struct image_t* details_img = new image_t;
    details_img->width = width;
    details_img->height = height;

    details_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        details_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            details_img->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++) {
                details_img->image_pixels[i][j][k] = 
                abs(input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k]);
            }
        }
    }
    return details_img;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image) {
    int width = input_image->width;
    int height = input_image->height;

    struct image_t* sharpened_image = new image_t;
    sharpened_image->width = width;
    sharpened_image->height = height;

    sharpened_image->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        sharpened_image->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            sharpened_image->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++) {
                sharpened_image->image_pixels[i][j][k] = min(255, max(0, (int)(input_image->image_pixels[i][j][k] + details_image->image_pixels[i][j][k])));
            }
        }
    }
    return sharpened_image;
}
