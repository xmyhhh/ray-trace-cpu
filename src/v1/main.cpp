
#include "ray.h"
#include "vec3.h"
#include "hittable.h"
#include "interval.h"
#include "camera.h"

#include <iostream>



int main() {

	// Image

	auto aspect_ratio = 16.0 / 9.0;
	int image_width = 128;

	// Calculate the image height, and ensure that it's at least 1.
	int image_height = static_cast<int>(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;


	// World

	hittable_list world;

	world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));

	// Camera

	camera cam;



	cam.render(world);


	std::clog << "\rDone.                 \n";
	std::cin;
}