#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <atomic>
#include "libppm.h" // Assumed to be your image processing library
#include <chrono>

using namespace std::chrono;
using namespace std;

// Declare atomic flags for synchronization
std::atomic<bool> s1_done(false);
std::atomic<bool> s2_done(false);

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Struct to pass data to threads
struct thread_data {
    struct image_t* input_image;
    struct image_t* smoothened_image;
    struct image_t* details_image;
    struct image_t* sharpened_image;
    char* output_file;
};


long long generatehash(struct image_t* image){
    long long sum=0;

    for(int i=0;i<image->height;i++)
	{
		for(int j=0;j<image->width;j++)
		{
            for(int k=0;k<3;k++)
            {
                //cout<<image->image_pixels[i][j][k]<<" ";
                // sum++;
                // sum+=image->image_pixels[i][j][k];
                sum^=image->image_pixels[i][j][k];
            }
            //cout<<endl;
        }
    }
    return sum;
}
void free_image(struct image_t *image) {
    if (!image) return;  // Safety check
    // Free each pixel's color array
    for (int i = 0; i < image->height; i++) {
        for (int j = 0; j < image->width; j++) {
            delete[] image->image_pixels[i][j];
        }
        // Free each row of pixels
        delete[] image->image_pixels[i];
    }
    // Free the image_pixels array
    delete[] image->image_pixels;
    // Free the image struct itself
    delete image;
}
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
// Smoothening function (S1)


void* S1_smoothen_thread(void* arg) {
    // cout<<"s1"<<endl;

    thread_data* data = (thread_data*)arg;

    pthread_mutex_lock(&mutex1); // Locking to synchronize S1
    data->smoothened_image = S1_smoothen(data->input_image);
    // cout<<"s1_smooth"<<generatehash(data->smoothened_image)<<endl;
    pthread_mutex_unlock(&mutex1); // Unlocking after smoothen is done

    s1_done.store(true); // Indicate that S1 is done
    pthread_exit(nullptr);
}

// Detail extraction function (S2)
void* S2_find_details_thread(void* arg) {

    thread_data* data = (thread_data*)arg;

    // Wait until S1 is done
    while (!s1_done.load()) {
        usleep(100); // Sleep for a while to avoid busy-waiting
    }

    pthread_mutex_lock(&mutex2); // Locking for S2 synchronization
    // cout<<"s2_smooth"<<generatehash(data->smoothened_image)<<endl;

    data->details_image = S2_find_details(data->input_image, data->smoothened_image);
    // cout<<"s2_det"<<generatehash(data->details_image)<<endl;
    pthread_mutex_unlock(&mutex2);

    s2_done.store(true); // Indicate that S2 is done
    pthread_exit(nullptr);
}

// Sharpening function (S3)
void* S3_sharpen_thread(void* arg) {
    // cout<<"s3"<<endl;
    thread_data* data = (thread_data*)arg;

    // Wait until S2 is done
    while (!s2_done.load()) {
        usleep(100); // Sleep for a while to avoid busy-waiting
    }
    // cout<<"s3_det"<<generatehash(data->details_image)<<endl;
    // Perform sharpening
    data->sharpened_image = S3_sharpen(data->input_image, data->details_image);
    // cout<<"s3_sharp"<<generatehash(data->sharpened_image)<<endl;
    pthread_exit(nullptr);
}


int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: ./a.out <input_image_path> <output_image_path>" << std::endl;
        return 1;
    }
    
    // Start measuring time using chrono
    auto start = high_resolution_clock::now();
        struct image_t* input_image = read_ppm_file(argv[1]);

    for (int i = 0; i < 1000; ++i) {
        thread_data data;
        data.input_image = input_image;
        data.output_file = argv[2];

        // Reset atomic flags for each iteration
        s1_done.store(false);
        s2_done.store(false);

        // Create threads
        pthread_t thread1, thread2, thread3;

        // Create thread for S1 (smoothing)
        pthread_create(&thread1, nullptr, S1_smoothen_thread, &data);

        // Create thread for S2 (finding details)
        pthread_create(&thread2, nullptr, S2_find_details_thread, &data);

        // Create thread for S3 (sharpening)
        pthread_create(&thread3, nullptr, S3_sharpen_thread, &data);

        // Wait for threads to complete
        pthread_join(thread1, nullptr);
        pthread_join(thread2, nullptr);
        pthread_join(thread3, nullptr);

        // Free the images after each iteration
        // free_image(input_image);
        free_image(data.smoothened_image);
        free_image(data.details_image);
        if(i==999)
        {
    // Write the final sharpened image to the output file
    write_ppm_file(data.output_file, data.sharpened_image);

        }
        free_image(data.sharpened_image);
    }
        free_image(input_image);


    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end-start).count();
    std::cout << "Processing time: " << elapsed << " seconds" << std::endl;
    return 0;
}
