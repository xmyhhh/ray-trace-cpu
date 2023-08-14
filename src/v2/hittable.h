#ifndef HITTABLE_H
#define HITTABLE_H
#include <memory>
#include<vector>

#include "ray.h"
#include "interval.h"
#include "aabb.h"


using std::shared_ptr;
using std::make_shared;

class material;

class hit_record {
public:
	point3 p;
	vec3 normal;
	double t;

	double u;
	double v;

	bool front_face;
	std::shared_ptr<material> mat;

	void set_face_normal(const ray& r, const vec3& outward_normal) {
		// Sets the hit record normal vector.
		// NOTE: the parameter `outward_normal` is assumed to have unit length.
		front_face = dot(r.direction(), outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};

class hittable {
protected:
	aabb bbox;

public:
	virtual ~hittable() = default;

	aabb bounding_box() const { return bbox; }

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

class hittable_list : public hittable {
public:
	std::vector<shared_ptr<hittable>> objects;

	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) { add(object); }

	void clear() { objects.clear(); }

	void add(shared_ptr<hittable> object) {
		objects.push_back(object);
		bbox = aabb(bbox, object->bounding_box());
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		auto closest_so_far = ray_t.max;

		for (const auto& object : objects) {
			if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}

		return hit_anything;
	}
};

class sphere : public hittable {
public:
	sphere(point3 _center, double _radius, std::shared_ptr<material> _material) : center(_center), radius(_radius), mat(_material) {
		auto rvec = vec3(radius, radius, radius);
		bbox = aabb(center - rvec, center + rvec);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec)  const override {
		vec3 oc = r.origin() - center;
		auto a = r.direction().length_squared();
		auto half_b = dot(oc, r.direction());
		auto c = oc.length_squared() - radius * radius;

		auto discriminant = half_b * half_b - a * c;
		if (discriminant < 0) return false;


		auto sqrtd = sqrt(discriminant);

		// Find the nearest root that lies in the acceptable range.
		auto root = (-half_b - sqrtd) / a;
		if (!ray_t.surrounds(root)) {
			root = (-half_b + sqrtd) / a;
			if (!ray_t.surrounds(root))
				return false;
		}

		rec.t = root;
		rec.p = r.at(rec.t);
		rec.mat = mat;

		vec3 outward_normal = (rec.p - center) / radius;
		rec.set_face_normal(r, outward_normal);
		get_sphere_uv(outward_normal, rec.u, rec.v);
		return true;
	}

	static void get_sphere_uv(const point3& p, double& u, double& v) {
		// p: a given point on the sphere of radius one, centered at the origin.
		// u: returned value [0,1] of angle around the Y axis from X=-1.
		// v: returned value [0,1] of angle from Y=-1 to Y=+1.
		//     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
		//     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
		//     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

		auto theta = acos(-p.y());
		auto phi = atan2(-p.z(), p.x()) + pi;

		u = phi / (2 * pi);
		v = theta / pi;
	}

private:
	point3 center;
	double radius;
	std::shared_ptr<material> mat;
};


class quad : public hittable {

public:
	quad(const point3& _Q, const vec3& _u, const vec3& _v, shared_ptr<material> m)
		: Q(_Q), u(_u), v(_v), mat(m)
	{
		set_bounding_box();

		auto n = cross(u, v);
		normal = unit_vector(n);
		D = dot(normal, Q);
		w = n / dot(n, n);
	}

	virtual void set_bounding_box() {
		bbox = aabb(Q, Q + u + v).pad();
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec)  const override {
		auto denom = dot(normal, r.direction());

		// No hit if the ray is parallel to the plane.
		if (fabs(denom) < 1e-8)
			return false;

		// Return false if the hit point parameter t is outside the ray interval.
		auto t = (D - dot(normal, r.origin())) / denom;
		if (!ray_t.contains(t))
			return false;

		// Determine the hit point lies within the planar shape using its plane coordinates.
		auto intersection = r.at(t);
		vec3 planar_hitpt_vector = intersection - Q;
		auto alpha = dot(w, cross(planar_hitpt_vector, v));
		auto beta = dot(w, cross(u, planar_hitpt_vector));

		if (!is_interior(alpha, beta, rec))
			return false;

		rec.t = t;
		rec.p = intersection;
		rec.mat = mat;
		rec.set_face_normal(r, normal);

		return true;
	}

private:
	point3 Q;
	vec3 u, v;
	shared_ptr<material> mat;

	vec3 normal;
	double D;
	vec3 w;

	bool is_interior(double a, double b, hit_record& rec) const {
		// Given the hit point in plane coordinates, return false if it is outside the
		// primitive, otherwise set the hit record UV coordinates and return true.

		if ((a < 0) || (1 < a) || (b < 0) || (1 < b))
			return false;

		rec.u = a;
		rec.v = b;
		return true;
	}
};

shared_ptr<hittable_list> box(const point3& a, const point3& b, shared_ptr<material> mat)
{
	// Returns the 3D box (six sides) that contains the two opposite vertices a & b.

	auto sides = make_shared<hittable_list>();

	// Construct the two opposite vertices with the minimum and maximum coordinates.
	auto min = point3(fmin(a.x(), b.x()), fmin(a.y(), b.y()), fmin(a.z(), b.z()));
	auto max = point3(fmax(a.x(), b.x()), fmax(a.y(), b.y()), fmax(a.z(), b.z()));

	auto dx = vec3(max.x() - min.x(), 0, 0);
	auto dy = vec3(0, max.y() - min.y(), 0);
	auto dz = vec3(0, 0, max.z() - min.z());

	sides->add(make_shared<quad>(point3(min.x(), min.y(), max.z()), dx, dy, mat)); // front
	sides->add(make_shared<quad>(point3(max.x(), min.y(), max.z()), -dz, dy, mat)); // right
	sides->add(make_shared<quad>(point3(max.x(), min.y(), min.z()), -dx, dy, mat)); // back
	sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()), dz, dy, mat)); // left
	sides->add(make_shared<quad>(point3(min.x(), max.y(), max.z()), dx, -dz, mat)); // top
	sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()), dx, dz, mat)); // bottom

	return sides;
}

class translate : public hittable {
public:
	translate(shared_ptr<hittable> p, const vec3& displacement)
		: object(p), offset(displacement)
	{
		bbox = object->bounding_box() + offset;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Move the ray backwards by the offset
		ray offset_r(r.origin() - offset, r.direction(), r.time());

		// Determine where (if any) an intersection occurs along the offset ray
		if (!object->hit(offset_r, ray_t, rec))
			return false;

		// Move the intersection point forwards by the offset
		rec.p += offset;

		return true;
	}

private:
	shared_ptr<hittable> object;
	vec3 offset;
	aabb bbox;
};

class rotate_y : public hittable {
public:
	rotate_y(shared_ptr<hittable> p, double angle) : object(p) {
		auto radians = degrees_to_radians(angle);
		sin_theta = sin(radians);
		cos_theta = cos(radians);
		bbox = object->bounding_box();

		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

					auto newx = cos_theta * x + sin_theta * z;
					auto newz = -sin_theta * x + cos_theta * z;

					vec3 tester(newx, y, newz);

					for (int c = 0; c < 3; c++) {
						min[c] = fmin(min[c], tester[c]);
						max[c] = fmax(max[c], tester[c]);
					}
				}
			}
		}

		bbox = aabb(min, max);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Change the ray from world space to object space
		auto origin = r.origin();
		auto direction = r.direction();

		origin[0] = cos_theta * r.origin()[0] - sin_theta * r.origin()[2];
		origin[2] = sin_theta * r.origin()[0] + cos_theta * r.origin()[2];

		direction[0] = cos_theta * r.direction()[0] - sin_theta * r.direction()[2];
		direction[2] = sin_theta * r.direction()[0] + cos_theta * r.direction()[2];

		ray rotated_r(origin, direction, r.time());

		// Determine where (if any) an intersection occurs in object space
		if (!object->hit(rotated_r, ray_t, rec))
			return false;

		// Change the intersection point from object space to world space
		auto p = rec.p;
		p[0] = cos_theta * rec.p[0] + sin_theta * rec.p[2];
		p[2] = -sin_theta * rec.p[0] + cos_theta * rec.p[2];

		// Change the normal from object space to world space
		auto normal = rec.normal;
		normal[0] = cos_theta * rec.normal[0] + sin_theta * rec.normal[2];
		normal[2] = -sin_theta * rec.normal[0] + cos_theta * rec.normal[2];

		rec.p = p;
		rec.normal = normal;

		return true;
	}

private:
	shared_ptr<hittable> object;
	double sin_theta;
	double cos_theta;
	aabb bbox;
};

#endif


