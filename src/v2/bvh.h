#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include "hittable.h"


class bvh_node : public hittable {
public:
	bvh_node(const hittable_list& list) : bvh_node(list.objects, 0, list.objects.size()) {}

	bvh_node(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end) {


		auto objects = src_objects;
		int axis = static_cast<int>(random_double(0, 3));

		auto comparator = (axis == 0) ? box_x_compare
			: (axis == 1) ? box_y_compare
			: box_z_compare;

		auto object_span = end - start;

		if (object_span == 1)
			left = right = objects[start];
		else if (object_span == 2) {
			left = objects[start];
			right = objects[start + 1];
		}
		else {
			//如果不排序，bvh的之间会有很多重叠，hit测试时容易左右都命中，耗时
			std::sort(objects.begin() + start, objects.begin() + end, comparator);

			auto mid = start + object_span / 2;
			left = make_shared<bvh_node>(objects, start, mid);
			right = make_shared<bvh_node>(objects, mid, end);
		}

		bbox = aabb(left->bounding_box(), right->bounding_box());
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		if (!bbox.hit(r, ray_t))
			return false;

		bool hit_left = left->hit(r, ray_t, rec);
		bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

		return hit_left || hit_right;
	}

private:
	shared_ptr<hittable> left;
	shared_ptr<hittable> right;
	aabb bbox;


	static bool box_compare(
		const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index
	) {
		return a->bounding_box().axis(axis_index).min < b->bounding_box().axis(axis_index).min;
	}

	static bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 0);
	}

	static bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 1);
	}

	static bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 2);
	}
};

#endif