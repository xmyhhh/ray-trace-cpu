
#include "ray.h"
#include "vec3.h"
#include "hittable.h"
#include "interval.h"
#include "camera.h"
#include "material.h"
#include <iostream>
#include "bvh.h"
#include <windows.h>

#include <string>




void random_spheres() {

	// Image

	auto aspect_ratio = 16.0 / 9.0;
	int image_width = 128;

	// Calculate the image height, and ensure that it's at least 1.
	int image_height = static_cast<int>(image_width / aspect_ratio);
	image_height = (image_height < 1) ? 1 : image_height;


	// World

	hittable_list world;

	auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_double();
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else if (choose_mat < 0.95) {
					// metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_double(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));


	auto p = make_shared<bvh_node>(world);

	world = hittable_list(p);

	// Camera

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 1920 / 2;
	cam.samples_per_pixel = 50;
	cam.max_depth = 50;

	cam.vfov = 20;
	cam.lookfrom = point3(13, 2, 3);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0.6;
	cam.focus_dist = 10.0;
	cam.file_name = "v2_random_spheres.ppm";

	cam.render(world);


}

void earth() {
	auto earth_texture = make_shared<image_texture>("earthmap.jpg");
	auto earth_surface = make_shared<lambertian>(earth_texture);
	auto globe = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 400;
	cam.samples_per_pixel = 100;
	cam.max_depth = 50;

	cam.vfov = 20;
	cam.lookfrom = point3(0, 0, 12);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;
	cam.file_name = "earth.ppm";

	cam.render(hittable_list(globe));
}


void draw_quad() {
	hittable_list world;

	// Materials
	auto left_red = make_shared<lambertian>(color(1.0, 0.2, 0.2));
	auto back_green = make_shared<lambertian>(color(0.2, 1.0, 0.2));
	auto right_blue = make_shared<lambertian>(color(0.2, 0.2, 1.0));
	auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
	auto lower_teal = make_shared<lambertian>(color(0.2, 0.8, 0.8));

	// Quads
	world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), left_red));
	world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
	world.add(make_shared<quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
	world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
	world.add(make_shared<quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4), lower_teal));

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = 400;
	cam.samples_per_pixel = 100;
	cam.max_depth = 50;

	cam.vfov = 80;
	cam.lookfrom = point3(0, 0, 9);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;
	cam.file_name = "quad.ppm";
	cam.render(world);
}

void draw_cornell_box() {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 330, 165), white);
	box1 = make_shared<rotate_y>(box1, 15);
	box1 = make_shared<translate>(box1, vec3(265, 0, 295));
	world.add(box1);

	shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 165, 165), white);
	box2 = make_shared<rotate_y>(box2, -18);
	box2 = make_shared<translate>(box2, vec3(130, 0, 65));
	world.add(box2);
	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = 600;
	cam.samples_per_pixel = 200;
	cam.max_depth = 50;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(278, 278, -800);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;
	cam.file_name = "cornell_box.ppm";
	cam.render(world);
}

int main() {

	__int64 begin = GetTickCount();

	//random_spheres();
	//earth();
	//draw_quad();
	draw_cornell_box();

	auto end = GetTickCount() - begin;
	std::clog << "\rDone.      " + std::to_string(end / 1000.0) + "           \n";

	int a;
	std::cin >> a;
}
