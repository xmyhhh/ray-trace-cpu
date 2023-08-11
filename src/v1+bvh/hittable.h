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

	aabb bounding_box() const  { return bbox; }

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
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

		return true;
	}

private:
	point3 center;
	double radius;
	std::shared_ptr<material> mat;
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


#endif


