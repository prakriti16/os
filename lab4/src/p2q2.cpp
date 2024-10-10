#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <wait.h>
#include <cstring>
#include <cstdint>
#include <chrono>
#include "libppm.h"  //includes definition of `image_t`
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
    shmid1 = shmget(IPC_PRIVATE, sizeof(struct image_t), IPC_CREAT | 0666); //s1 to s2 shared mem
    if (shmid1 < 0) {
        perror("shmget");
        exit(1);
    }

    shmid2 = shmget(IPC_PRIVATE, sizeof(struct image_t), IPC_CREAT | 0666); //s2 to s3 shared mem
    if (shmid2 < 0) {
        perror("shmget");
        exit(1);
    }

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666); // sem sets ; sem_num 0 & sem_num 1
    if (semid < 0) {
        perror("semget");
        exit(1);
    }

    union semun sem_union; //initial to 0, locked
    sem_union.val = 0;
    for (int i = 0; i < 2; ++i) {
        semctl(semid, i, SETVAL, sem_union);
    }
}

void sem_signal(int sem_num) { //increm sem val,resource avail
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = 1;  //unlock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void sem_wait(int sem_num) { //decremen sem val
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -1;  //lock
    op.sem_flg = 0;
    semop(semid, &op, 1);
}

void run_parallel(struct image_t* input_image, const char* output_filename) {
    setup_ipc();

    pid_t pid_s1 = fork();
    if (pid_s1 == 0) { //s1 process
        struct image_t *shm_s1 = (struct image_t*)shmat(shmid1, NULL, 0); //attaches shared mem shm1 to curr process'(s1) address space
        for (int i = 0; i < 10; ++i) {  // reduced to 10 iterations for testing
            cout << "S1: Iteration " << i << endl; // Debug output
            struct image_t* smooth_img = S1_smoothen(input_image);  // Process smoothening
            memcpy(shm_s1, smooth_img, sizeof(struct image_t)); // Copy result to shared memory
            sem_signal(0);  //signals s2 , semaphore assoc with sem_num 0 is ++
            delete smooth_img; // Free the temporary smoothened image
        }
        shmdt(shm_s1);
        cout << "S1: Finished\n"; // Debug output
        exit(0);
    }

    pid_t pid_s2 = fork();
    if (pid_s2 == 0) { //process s2
        struct image_t *shm_s1 = (struct image_t*)shmat(shmid1, NULL, 0); //attaches shared mem sh1 (for s2 to access it)
        struct image_t *shm_s2 = (struct image_t*)shmat(shmid2, NULL, 0); // attaches shared mem sh2 to s2's add space(for s2 to write in it)
        for (int i = 0; i < 10; ++i) { // reduced to 10 iterations for testing
            sem_wait(0);  //waits for s1 to finish
            cout << "S2: Iteration " << i << endl; // Debug output
            struct image_t* details_img = S2_find_details(input_image, shm_s1); // Find details
            memcpy(shm_s2, details_img, sizeof(struct image_t)); // Copy result to shared memory
            sem_signal(1);  
            delete details_img; // Free the temporary details image
        }
        shmdt(shm_s1); //detach shm1 & 2
        shmdt(shm_s2);
        cout << "S2: Finished\n"; // Debug output
        exit(0);
    }

    pid_t pid_s3 = fork(); //process 3
    if (pid_s3 == 0) {
        struct image_t *shm_s2 = (struct image_t*)shmat(shmid2, NULL, 0);
        struct image_t *sharpened_image = nullptr;
        for (int i = 0; i < 10; ++i) { // reduced to 10 iterations for testing
            sem_wait(1); // wait for s2 to finish
            cout << "S3: Iteration " << i << endl; // Debug output
            sharpened_image = S3_sharpen(input_image, shm_s2); // Sharpen the image
        }
        write_ppm_file((char*)output_filename, sharpened_image); // Write output
        delete sharpened_image; // Free the sharpened image
        cout << "S3: Finished\n"; // Debug output
        exit(0);
    }
    
    // Parent process waits for child processes
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // Cleanup IPC
    shmctl(shmid1, IPC_RMID, NULL); // Clean up shared memory
    shmctl(shmid2, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, NULL); // Clean up semaphores
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

// S1, S2, S3 function definitions from your previous code
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

    struct image_t* detail_img = new image_t;
    detail_img->width = width;
    detail_img->height = height;

    detail_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        detail_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            detail_img->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++) {
                detail_img->image_pixels[i][j][k] = abs(input_image->image_pixels[i][j][k] - smoothened_image->image_pixels[i][j][k]);
            }
        }
    }

    return detail_img;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image) {
    int width = input_image->width;
    int height = input_image->height;

    struct image_t* sharpen_img = new image_t;
    sharpen_img->width = width;
    sharpen_img->height = height;

    sharpen_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        sharpen_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            sharpen_img->image_pixels[i][j] = new uint8_t[3];

            for (int k = 0; k < 3; k++) {
                sharpen_img->image_pixels[i][j][k] = input_image->image_pixels[i][j][k] + details_image->image_pixels[i][j][k];
                if (sharpen_img->image_pixels[i][j][k] > 255) {
                    sharpen_img->image_pixels[i][j][k] = 255;
                }
            }
        }
    }

    return sharpen_img;
}
