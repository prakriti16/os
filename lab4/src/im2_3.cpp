#include <bits/stdc++.h>
#include <atomic>
#include <thread>
#include <chrono>
#include "libppm.h"  
using namespace std;
using namespace std::chrono;

atomic<bool> s1_done(false);
atomic<bool> s2_done(false);

struct image_t* S1_smoothen(struct image_t* input_image);
struct image_t* S2_find_details(struct image_t* input_image, struct image_t* smoothened_image);
struct image_t* S3_sharpen(struct image_t* input_image, struct image_t* details_image);

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
    for(int i=0;i<image->height;i++) {
        for(int j=0;j<image->width;j++) {
            for(int k=0;k<3;k++) {
                uint8_t pixel_value = image->image_pixels[i][j][k];
                sum ^= (pixel_value << (k + (i % 8))) ^ (pixel_value >> (j % 8));
            }
        }
    }
    return sum;
}
void run_parallel(struct image_t* input_image, const char* output_filename)
{
    int width = input_image->width;
    int height = input_image->height;

    // Allocate images once, but free them within the loop
    struct image_t* smooth_img = nullptr;
    struct image_t* detail_img = nullptr;
    struct image_t* sharpen_img = nullptr;

    // Thread for S1 (smoothen)
    thread t1([&]() {
        for (int i = 0; i < 1000; ++i) {
            struct image_t* local_smooth_img = S1_smoothen(input_image);
            if (local_smooth_img == nullptr) {
                cerr << "Error in S1: Smoothened image is null!" << endl;
                return;
            }

            // Cleanup previous smooth_img before storing new one
            if (smooth_img) {
                cleanup_image_t(smooth_img);
            }

            smooth_img = local_smooth_img;  // Store the new smooth image

            cout << "S1 smooth image hash: " << generatehash(local_smooth_img) << " in iteration " << i << endl;
            s1_done.store(true, memory_order_release);  // Signal that S1 is done
            while (s1_done.load(memory_order_acquire)) {
                std::this_thread::sleep_for(chrono::milliseconds(1));  // Reduced spin-wait CPU usage
            }
        }
    });

    // Thread for S2 (find details)
    thread t2([&]() {
        for (int i = 0; i < 1000; ++i) {
            while (!s1_done.load(memory_order_acquire)) {
                std::this_thread::sleep_for(chrono::milliseconds(1));  // Reduced spin-wait CPU usage
            }
            cout << "S2 smooth image hash: " << generatehash(smooth_img) << " in iteration " << i << endl;
            
            struct image_t* local_detail_img = S2_find_details(input_image, smooth_img);
            if (local_detail_img == nullptr) {
                cerr << "Error in S2: Details image is null!" << endl;
                return;
            }

            // Cleanup previous detail_img before storing new one
            if (detail_img) {
                cleanup_image_t(detail_img);
            }

            detail_img = local_detail_img;  // Store the new details image

            cout << "S2 details image hash: " << generatehash(local_detail_img) << " in iteration " << i << endl;
            s1_done.store(false, memory_order_release);  // Reset S1 flag for the next iteration
            s2_done.store(true, memory_order_release);   // Signal that S2 is done
        }
    });

    // Thread for S3 (sharpen)
    thread t3([&]() {
        for (int i = 0; i < 1000; ++i) {
            while (!s2_done.load(memory_order_acquire)) {
                std::this_thread::sleep_for(chrono::milliseconds(1));  // Reduced spin-wait CPU usage
            }
            cout << "S3 details image hash: " << generatehash(detail_img) << " in iteration " << i << endl;
            
            struct image_t* local_sharpen_img = S3_sharpen(input_image, detail_img);
            if (local_sharpen_img == nullptr) {
                cerr << "Error in S3: Sharpened image is null!" << endl;
                return;
            }

            // Cleanup previous sharpen_img before storing new one
            if (sharpen_img) {
                cleanup_image_t(sharpen_img);
            }

            sharpen_img = local_sharpen_img;  // Store the new sharpened image

            cout << "S3 sharpened image hash: " << generatehash(local_sharpen_img) << " in iteration " << i << endl;
            if (i == 999) {
                write_ppm_file((char*)output_filename, local_sharpen_img);  // Write the final output image
            }

            s2_done.store(false, memory_order_release);  // Reset S2 flag for the next iteration
        }
    });

    t1.join();
    t2.join();
    t3.join();

    // Clean up images at the end
    if (smooth_img) {
        cleanup_image_t(smooth_img);
    }
    if (detail_img) {
        cleanup_image_t(detail_img);
    }
    if (sharpen_img) {
        cleanup_image_t(sharpen_img);
    }
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
    if (input_image == nullptr) {
        cerr << "Error: Could not read input image!" << endl;
        exit(1);
    }

    auto start = high_resolution_clock::now();
    run_parallel(input_image, argv[2]);
    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end - start).count();
    cout << "Processing time: " << elapsed << " seconds" << endl;

    cleanup_image_t(input_image);
    return 0;
}

// Smoothening function (S1)
struct image_t* S1_smoothen(struct image_t *input_image) {
    if (!input_image) return nullptr;  // Safety check for null input

    int width = input_image->width;
    int height = input_image->height;

    struct image_t* smooth_img = new image_t;
    smooth_img->width = width;
    smooth_img->height = height;

    smooth_img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; i++) {
        smooth_img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; j++) {
            smooth_img->image_pixels[i][j] = new uint8_t[3];
            for (int k = 0; k < 3; k++) {
                smooth_img->image_pixels[i][j][k] = input_image->image_pixels[i][j][k];
            }
        }
    }

    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            for (int k = 0; k < 3; k++) {
                smooth_img->image_pixels[i][j][k] = (
                    input_image->image_pixels[i-1][j][k] + input_image->image_pixels[i][j-1][k] +
                    input_image->image_pixels[i+1][j][k] + input_image->image_pixels[i][j+1][k] +
                    input_image->image_pixels[i-1][j-1][k] + input_image->image_pixels[i+1][j+1][k] +
                    input_image->image_pixels[i-1][j+1][k] + input_image->image_pixels[i+1][j-1][k] +
                    input_image->image_pixels[i][j][k]
                ) / 9;
            }
        }
    }

    return smooth_img;
}

// Finding details function (S2)
struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image) {
    if (!input_image || !smoothened_image) return nullptr;  // Safety check

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

// Sharpening function (S3)
struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image) {
    if (!input_image || !details_image) return nullptr;  // Safety check

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
