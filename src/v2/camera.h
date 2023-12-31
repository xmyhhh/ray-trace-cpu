#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"
#include "vec3.h"
#include "hittable.h"
#include <fstream>
#include<ppl.h>
#include"material.h"

void write_color(std::ostream& out, vec3 pixel_color, int samples_per_pixel) {
	//Images with data that are written without being transformed are said to be in linear space, whereas images that are transformed are said to be in gamma space.
	auto linear_to_gamma = [](vec3 linear_component)
	{
		return linear_component.squared();
	};
	pixel_color = linear_to_gamma(pixel_color / samples_per_pixel);

	out << static_cast<int>(255.999 * pixel_color.x()) << ' '
		<< static_cast<int>(255.999 * pixel_color.y()) << ' '
		<< static_cast<int>(255.999 * pixel_color.z()) << '\n';
}


class camera {
public:
	std::string file_name = "example.ppm";
	double aspect_ratio = 1.0;  // Ratio of image width over height
	int    image_width = 100;  // Rendered image width in pixel count
	int    samples_per_pixel = 10;   // Count of random samples for each pixel
	int    max_depth = 10;   // Maximum number of ray bounces into scene

	double vfov = 90;              // Vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, -1);  // Point camera is looking from
	point3 lookat = point3(0, 0, 0);   // Point camera is looking at
	vec3   vup = vec3(0, 1, 0);     // Camera-relative "up" direction

	double defocus_angle = 0;  // Variation angle of rays through each pixel
	double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

	point3 defocus_disk_sample() const {
		// Returns a random point in the camera defocus disk.
		auto   random_in_unit_disk = []() {
			while (true) {
				auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
				if (p.length_squared() < 1)
					return p;
			}
		};

		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	ray get_ray(int i, int j) const {
		// Get a randomly sampled camera ray for the pixel at location i,j.

		auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
		auto pixel_sample = pixel_center + pixel_sample_square();

		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto ray_direction = pixel_sample - ray_origin;
		auto ray_time = random_double();

		return ray(ray_origin, ray_direction, ray_time);
	}

	vec3 pixel_sample_square() const {
		// Returns a random point in the square surrounding a pixel at the origin.
		auto px = -0.5 + random_double();
		auto py = -0.5 + random_double();
		return (px * pixel_delta_u) + (py * pixel_delta_v);
	}

	void render(const hittable& world) {
		initialize();

		std::ofstream myfile;
		myfile.open(file_name, std::ios::out | std::ios::binary | std::ios::trunc);

		myfile << "P3\n" << image_width << " " << image_height << "\n255\n";
		std::vector<color> image(image_width * image_height);

		Concurrency::parallel_for(0, image_height, [&](int j) {
			/*std::clog << "\rScanlines remaining: " << (remine - -) << ' ' << std::flush;*/
			for (int i = 0; i < image_width; ++i) {
				color pixel_color(0, 0, 0);
				for (int sample = 0; sample < samples_per_pixel; ++sample) {
					ray r = get_ray(i, j);
					pixel_color += ray_color(r, max_depth, world);
				}
				//write_color(myfile, pixel_color, samples_per_pixel);
				image[(j * image_width + i)] = pixel_color;
			}
			});

		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; ++i) {
				write_color(myfile, image[(j * image_width + i)], samples_per_pixel);
			}
		}
	}
	color  background = color(0.70, 0.80, 1.00);;               // Scene background color
private:
	int    image_height;   // Rendered image height
	point3 center;         // Camera center
	point3 pixel00_loc;    // Location of pixel 0, 0
	vec3   pixel_delta_u;  // Offset to pixel to the right
	vec3   pixel_delta_v;  // Offset to pixel below
	vec3   u, v, w;        // Camera frame basis vectors
	

	vec3   defocus_disk_u;  // Defocus disk horizontal radius
	vec3   defocus_disk_v;  // Defocus disk vertical radius

	void initialize() {
		image_height = static_cast<int>(image_width / aspect_ratio);
		image_height = (image_height < 1) ? 1 : image_height;

		center = lookfrom;

		// Determine viewport dimensions.
		auto theta = degrees_to_radians(vfov);
		auto h = tan(theta / 2);
		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

		// Calculate the u,v,w unit basis vectors for the camera coordinate frame.
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);

		// Calculate the vectors across the horizontal and down the vertical viewport edges.
		vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
		vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

		// Calculate the horizontal and vertical delta vectors from pixel to pixel.
		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		// Calculate the location of the upper left pixel.
		auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		// Calculate the camera defocus disk basis vectors.
		auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}

	color ray_color(const ray& r, int max_depth, const hittable& world) const {
		hit_record rec;

		// If we've exceeded the ray bounce limit, no more light is gathered.
		if (max_depth <= 0) {
			return color(0, 0, 0);
		}


		// If the ray hits nothing, return the background color.
		if (!world.hit(r, interval(0.001, infinity), rec))
			return background;

		ray scattered;
		color attenuation;
		color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

		if (!rec.mat->scatter(r, rec, attenuation, scattered))
			return color_from_emission;

		color color_from_scatter = attenuation * ray_color(scattered, max_depth - 1, world);

		return color_from_emission + color_from_scatter;
	}
};

#endif