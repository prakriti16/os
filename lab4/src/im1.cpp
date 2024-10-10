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

size_t estimate_memory_usage(struct image_t* image) {
    size_t total_size = 0;
    total_size += sizeof(struct image_t); // Size of the structure itself
    total_size += sizeof(uint8_t**) * image->height; // For the rows of image_pixels
    for (int i = 0; i < image->height; ++i) {
        total_size += sizeof(uint8_t*) * image->width; // For the width (pixel arrays)
        for (int j = 0; j < image->width; ++j) {
            total_size += sizeof(uint8_t) * 3; // For RGB values
        }
    }
    return total_size;
}

void print_memory_usage(struct image_t* image) {
    std::cout << "Estimated memory usage: " << estimate_memory_usage(image) << " bytes" << std::endl;
}
int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }
    clock_t start, end;
    clock_t s1,s2,s3,e1,e2,e3;
    start = clock(); 
    struct image_t *input_image = read_ppm_file(argv[1]);
    struct image_t *sharpened_image,*smoothened_image,*details_image;
    double s1_time,s2_time,s3_time;
    for(int i=0;i<1000;i++){
        if(i==0){
            s1=clock();
        }
        smoothened_image = S1_smoothen(input_image);
        if(i==999){
            e1=clock();
        }
        //print_memory_usage(smoothened_image);
        if(i==0){
            s2=clock();
        }
        details_image = S2_find_details(input_image, smoothened_image);
        if(i==999){
            e2=clock();
        }
        //print_memory_usage(details_image);
        if(i==0){
            s3=clock();
        }
        sharpened_image = S3_sharpen(input_image, details_image);
        if(i==999){
            e3=clock();
        }
        free_image(smoothened_image);
        free_image(details_image);
        if(i!=999){
            free_image(sharpened_image);
        }
    }
    write_ppm_file(argv[2], sharpened_image);
    end = clock();
    double totruntime = double(end - start) / double(CLOCKS_PER_SEC);
    double s1t = double(e1 - s1) / double(CLOCKS_PER_SEC);
    double s2t = double(e2 - s2) / double(CLOCKS_PER_SEC);
    double s3t = double(e3 - s3) / double(CLOCKS_PER_SEC);
    cout<<"time (in seconds)"<<endl<<"s1 :"<<s1t<<endl<<"s2 : "<<s2t<<endl<<"s3 :"<<s3t<<endl<<"Total execution time: "<<totruntime<<endl;
    //cout << "-----------------" << endl;
    return 0;
}
