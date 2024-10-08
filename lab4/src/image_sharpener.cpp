#include <iostream>
#include "libppm.h"
#include <cstdint>
#include <chrono>

using namespace std;
using namespace std::chrono;

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

	return sharpen_img; //TODO remove this line when adding your code
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }

    auto start = high_resolution_clock::now();
    struct image_t *input_image = read_ppm_file(argv[1]);
    auto end = high_resolution_clock::now();
    auto read_time = duration_cast<milliseconds>(end - start).count();

    start = high_resolution_clock::now();
    struct image_t *smoothened_image = S1_smoothen(input_image);
    end = high_resolution_clock::now();
    auto s1_time = duration_cast<milliseconds>(end - start).count();

    start = high_resolution_clock::now();
    struct image_t *details_image = S2_find_details(input_image, smoothened_image);
    end = high_resolution_clock::now();
    auto s2_time = duration_cast<milliseconds>(end - start).count();

    start = high_resolution_clock::now();
    struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
    end = high_resolution_clock::now();
    auto s3_time = duration_cast<milliseconds>(end - start).count();

    start = high_resolution_clock::now();
    write_ppm_file(argv[2], sharpened_image);
    end = high_resolution_clock::now();
    auto write_time = duration_cast<milliseconds>(end - start).count();

    cout << "File Read: " << read_time << endl << "S1 Smoothen: " << s1_time << endl<< "S2 Details: " << s2_time <<  endl<< "S3 Sharpen: " << s3_time <<  endl<< "File Write: " << write_time << endl;

    return 0;
}