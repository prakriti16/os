#include <iostream>
#include <unistd.h> // For pipe, fork, write, read
#include <sys/wait.h> // For wait
#include <vector>
#include <fcntl.h> // For file descriptors

#include "libppm.h"
#include <cstdint>
#include <chrono>

using namespace std::chrono;
using namespace std;

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

void s1(int pipefd[2], struct image_t* input_image) {
    close(pipefd[0]); // Close the read end of the pipe
    
    for (int i = 0; i < 1000; i++) {
        struct image_t* smoothened_image = S1_smoothen(input_image);
        // Write the smoothened image to the pipe
        write(pipefd[1], smoothened_image, sizeof(*smoothened_image)); // Serialize this properly based on the image structure
        free_image(smoothened_image);
    }
    close(pipefd[1]); // Close the write end when done
    exit(0);
}

void s2(int pipefd1[2], int pipefd2[2], struct image_t* input_image) {
    close(pipefd1[1]); // Close write end of first pipe
    close(pipefd2[0]); // Close read end of second pipe

    for (int i=0; i< 1000; i++) {
        struct image_t smoothened_image;

        // Read the smoothened image from the pipe
        read(pipefd1[0], &smoothened_image, sizeof(smoothened_image)); // Deserialize if needed
        
        struct image_t* details_image = S2_find_details(input_image, &smoothened_image);
        
        // Write the details image to the second pipe
        write(pipefd2[1], details_image, sizeof(*details_image)); // Serialize this properly
        free_image(details_image);
    }

    close(pipefd2[1]); // Close the write end when done
    close(pipefd1[0]);
    exit(0);
}

void s3(int pipefd[2], struct image_t* input_image, char* output_part2_1) {
    close(pipefd[1]); // Close the write end of the pipe

    for (int i=0; i<1000; i++) {
        struct image_t details_image;

        // Read the details image from the pipe
        read(pipefd[0], &details_image, sizeof(details_image)); // Deserialize properly
        
        struct image_t* sharpened_image = S3_sharpen(input_image, &details_image);
        
        // If it's the last iteration, write the sharpened image
        if (i==999) {
            write_ppm_file(output_part2_1, sharpened_image);
        }
        free_image(sharpened_image);
    }

    close(pipefd[0]);
    exit(0);
}
int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: ./a.out <input_image_path> <output_image_path>" << std::endl;
        return 1;
    }

    // Start measuring time using chrono
    auto start = high_resolution_clock::now();
    
    struct image_t* input_image = read_ppm_file(argv[1]);

    // Create two pipes for communication
    int pipefd1[2], pipefd2[2];

    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        std::cerr << "Pipe creation failed" << std::endl;
        return 1;
    }

    pid_t p1 = fork();
    if (p1 > 0) {
        pid_t p2 = fork();
        if (p2 > 0) {
            pid_t p3 = fork();
            if (p3 > 0) {
                // Parent process
                close(pipefd1[0]); // Close unused read end
                close(pipefd1[1]); // Close unused write end
                close(pipefd2[0]); // Close unused read end
                close(pipefd2[1]); // Close unused write end
                
                // Wait for child processes to finish
                wait(NULL);
                wait(NULL);
                wait(NULL);
            }
            else if (p3 == 0) {
                s3(pipefd2, input_image, argv[2]);
            }
        }
        else if (p2 == 0) {
            s2(pipefd1, pipefd2, input_image);
        }
    } else if (p1 == 0) {
        s1(pipefd1, input_image);
    }
    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<duration<double>>(end-start).count();
    std::cout << "Processing time: " << elapsed << " seconds" << std::endl;

    free_image(input_image);
    return 0;
}