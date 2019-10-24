#include <iostream>
#include <cmath>
#include "ppm.h"
#include "PerlinNoise.h"

int main() {
	// Define the size of the image
	unsigned int width = 2000, height = 2000;

	// Create an empty PPM image
	ppm image(width, height);

	// Create a PerlinNoise object with a random permutation vector generated with seed
	unsigned int seed = 237;
	PerlinNoise pn(seed);

	unsigned int kk = 0;
    double n_max = 0;
    double n_min = 10;
	// Visit every pixel of the image and assign a color generated with Perlin noise
	for(unsigned int i = 0; i < height; ++i) {     // y
		for(unsigned int j = 0; j < width; ++j) {  // x
			double x = (double)j/((double)width);
			double y = (double)i/((double)height);

			// Typical Perlin noise
			double n = pn.noise(20 * x, 20 * y, 0.8)/4.0;
            n += pn.noise(10 * x, 10 * y, 0.8)/1.34;
            if (i < 150 || i > 1850 || j < 150 || j > 1850){
                n += 0.4;
            }
            n -= 0.21;

            if ( n > n_max){
                n_max = n;
            }
            if (n < n_min){
                n_min = n;
            }

			// Map the values to the [0, 255] interval, for simplicity we use 
			// tones of grey
			image.r[kk] = floor(255 * n);
			image.g[kk] = floor(255 * n);
			image.b[kk] = floor(255 * n);
			kk++;
		}
	}

    std::cout << n_max << std::endl;
    std::cout << n_min << std::endl;

	// Save the image in a binary PPM file
	image.write("figure_7_P.ppm");

	return 0;
}
