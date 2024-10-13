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
                //sum+=image->image_pixels[i][j][k];
                sum^=image->image_pixels[i][j][k];
            }
            //cout<<endl;
        }
    }
    return sum;
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
    uint8_t* shm_s1 = (uint8_t*)shmat(shmid1, NULL, 0);
    if (shm_s1 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }
    auto s1 = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i)
    {
        struct image_t* smooth_img = S1_smoothen(input_image);

        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                for (int k = 0; k < 3; k++)
                    shm_s1[i * width * 3 + j * 3 + k] = smooth_img->image_pixels[i][j][k];
        cout<<"S1 smooth image hash: "<<generatehash(smooth_img)<<" in iteration "<<i<<endl;
        cleanup_image_t(smooth_img);
        sem_signal(0);
        sem_wait(1);
    }
    auto e1 = high_resolution_clock::now();
    double t1 = duration_cast<duration<double>>(e1-s1).count();
    cout << "s1: " << t1 << " seconds" << endl;
    shmdt(shm_s1);
    exit(0);
}


    pid_t pid_s2 = fork();
    if (pid_s2 == 0) // S2 process
{
    
    uint8_t* shm_s1 = (uint8_t*)shmat(shmid1, NULL, 0);
    uint8_t* shm_s2 = (uint8_t*)shmat(shmid2, NULL, 0);
    if (shm_s1 == (uint8_t*)(-1) || shm_s2 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }
    auto s2 = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i)
    {
        sem_wait(0); // wait for S1 to finish
        struct image_t* smoothened_image = convert_arr_to_image_t(shm_s1, width, height);
        sem_signal(1);
        cout<<"S2 smooth image hash: "<<generatehash(smoothened_image)<<" in iteration "<<i<<endl;

        struct image_t* details_img = S2_find_details(input_image, smoothened_image);
        cleanup_image_t(smoothened_image);
        
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                for (int k = 0; k < 3; k++)
                    shm_s2[i * width * 3 + j * 3 + k] = details_img->image_pixels[i][j][k];
        cout<<"S2 details image hash: "<<generatehash(details_img)<<" in iteration "<<i<<endl;
        cleanup_image_t(details_img);
        sem_signal(2);
        sem_wait(3);
    }
    auto e2 = high_resolution_clock::now();
    double t2 = duration_cast<duration<double>>(e2-s2).count();
    cout << "s2: " << t2 << " seconds" << endl;
    shmdt(shm_s1);
    shmdt(shm_s2);
    exit(0);
}


    pid_t pid_s3 = fork(); // S3 process
    if (pid_s3 == 0) // S3 process
{
    uint8_t* shm_s2 = (uint8_t*)shmat(shmid2, NULL, 0);
    if (shm_s2 == (uint8_t*)(-1))
    {
        perror("shmat");
        exit(1);
    }
    auto s3 = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i)
    {
        sem_wait(2); // Wait for S2 to finish
        struct image_t* details_image = convert_arr_to_image_t(shm_s2, width, height);
        sem_signal(3);
        cout<<"S3 details image hash: "<<generatehash(details_image)<<" in iteration "<<i<<endl;
        
        struct image_t* sharpened_image = S3_sharpen(input_image, details_image);
        cout<<"S3 sharpened image hash: "<<generatehash(details_image)<<" in iteration "<<i<<endl;
        cleanup_image_t(details_image);
        if(i==999){
            write_ppm_file((char*)output_filename, sharpened_image);
        }
        cleanup_image_t(sharpened_image);
    }
    auto e3 = high_resolution_clock::now();
    double t3 = duration_cast<duration<double>>(e3-s3).count();
    cout << "s3: " << t3 << " seconds" << endl;

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
    cout << "Program started." << endl;

    if (argc != 3)
    {
        cout << "Usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n";
        exit(0);
    }

    struct image_t* input_image = read_ppm_file(argv[1]);
    auto start = high_resolution_clock::now();
    run_parallel(input_image, argv[2]);
    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end-start).count();
    cout << "Processing time: " << elapsed << " seconds" << endl;

    return 0;
}

// Function definitions
struct image_t* S1_smoothen(struct image_t *input_image)
{
	// TODO

	// remember to allocate space for smoothened_image. See read_ppm_file() in libppm.c for some help.
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

	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			if(i==0||j==0||i==height-1||j==width-1){
				continue;
			}
			else{
				for(int k=0;k<3;k++)
				{
					smooth_img->image_pixels[i][j][k]=(input_image->image_pixels[i-1][j][k]+input_image->image_pixels[i][j-1][k]+input_image->image_pixels[i-1][j-1][k]+input_image->image_pixels[i+1][j][k]+input_image->image_pixels[i][j+1][k]+input_image->image_pixels[i+1][j+1][k]+input_image->image_pixels[i-1][j+1][k]+input_image->image_pixels[i+1][j-1][k]+input_image->image_pixels[i][j][k])/9;
				}
			}
		}
	}
	return smooth_img;
}

struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image)
{
	// TODO
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
                detail_img->image_pixels[i][j][k] = abs(input_image->image_pixels[i][j][k]-smoothened_image->image_pixels[i][j][k]);

            }
        }
    }

	return detail_img;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image)
{
	// TODO
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
                sharpen_img->image_pixels[i][j][k] =input_image->image_pixels[i][j][k]+details_image->image_pixels[i][j][k] ;
				if(sharpen_img->image_pixels[i][j][k]>255){
					sharpen_img->image_pixels[i][j][k]=255;
				}

            }
        }
    }

	return sharpen_img; 
}
