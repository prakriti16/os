#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <atomic>
#include "libppm.h" 
#include <chrono>

using namespace std::chrono;
using namespace std;

// Declare atomic flags for synchronization
std::atomic<bool> s1_done(false);
std::atomic<bool> s2_done(false);
std::atomic_flag lock_s1 = ATOMIC_FLAG_INIT;
std::atomic_flag lock_s2 = ATOMIC_FLAG_INIT;
void test_and_set_lock(std::atomic_flag& lock) {
    // Spin-lock until the flag is successfully set
    while (lock.test_and_set(std::memory_order_acquire)) {
        // Busy-wait until the lock is acquired
    }
}
void release_lock(std::atomic_flag& lock) {
    lock.clear(std::memory_order_release); // Release the lock
}


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
    thread_data* data = (thread_data*)arg;

    test_and_set_lock(lock_s1); // Replace mutex lock with test-and-set lock
    data->smoothened_image = S1_smoothen(data->input_image);
    //cout << "s1_smooth" << generatehash(data->smoothened_image) << endl;
    release_lock(lock_s1); // Replace mutex unlock with test-and-set unlock

    s1_done.store(true); // Indicate that S1 is done
    pthread_exit(nullptr);
}


void* S2_find_details_thread(void* arg) {
    thread_data* data = (thread_data*)arg;

    while (!s1_done.load()) {
        usleep(100); // Sleep for a while to avoid busy-waiting
    }

    test_and_set_lock(lock_s2); // Replace mutex lock with test-and-set lock
    //cout << "s2_smooth" << generatehash(data->smoothened_image) << endl;
    data->details_image = S2_find_details(data->input_image, data->smoothened_image);
    //cout << "s2_det" << generatehash(data->details_image) << endl;
    release_lock(lock_s2); // Replace mutex unlock with test-and-set unlock

    s2_done.store(true); // Indicate that S2 is done
    pthread_exit(nullptr);
}


void* S3_sharpen_thread(void* arg) {
    thread_data* data = (thread_data*)arg;

    while (!s2_done.load()) {
        usleep(100); // Sleep for a while to avoid busy-waiting
    }
    //cout<<"s3_det"<<generatehash(data->details_image)<<endl;
    data->sharpened_image = S3_sharpen(data->input_image, data->details_image);
    //cout<<"s3_sharp"<<generatehash(data->sharpened_image)<<endl;
    pthread_exit(nullptr);
}



int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: ./a.out <input_image_path> <output_image_path>" << std::endl;
        return 1;
    }
    //freopen("input.txt","w",stdout);
    
    auto start = high_resolution_clock::now();
        struct image_t* input_image = read_ppm_file(argv[1]);
    auto s1 = high_resolution_clock::now();
    auto s2 = high_resolution_clock::now();
    auto s3 = high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        thread_data data;
        data.input_image = input_image;
        data.output_file = argv[2];

        // Reset atomic flags for each iteration
        s1_done.store(false);
        s2_done.store(false);

        // Create threads
        pthread_t thread1, thread2, thread3;
        if(i==0){
            s1 = high_resolution_clock::now();
        }
        // Create thread for S1 (smoothing)
        pthread_create(&thread1, nullptr, S1_smoothen_thread, &data);
        if(i==999){
            auto e1 = high_resolution_clock::now();
            double t1 = duration_cast<duration<double>>(e1-s1).count();
            cout << "s1: " << t1 << " seconds" << endl;
        }
        
        if(i==0){
            s2 = high_resolution_clock::now();
        }
        // Create thread for S2 (finding details)
        pthread_create(&thread2, nullptr, S2_find_details_thread, &data);
        if(i==999){
            auto e2 = high_resolution_clock::now();
            double t2 = duration_cast<duration<double>>(e2-s2).count();
            cout << "s2: " << t2 << " seconds" << endl;
        }

        if(i==0){
            s3 = high_resolution_clock::now();
        }
        // Create thread for S3 (sharpening)
        pthread_create(&thread3, nullptr, S3_sharpen_thread, &data);
        if(i==999){
            auto e3 = high_resolution_clock::now();
            double t3 = duration_cast<duration<double>>(e3-s3).count();
            cout << "s3: " << t3 << " seconds" << endl;
        }

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
