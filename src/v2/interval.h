#ifndef INTERVAL_H
#define INTERVAL_H

class interval {
public:
	double min, max;

	interval() : min(+infinity), max(-infinity) {} // Default interval is empty

	interval(double _min, double _max) : min(_min), max(_max) {}

	interval(const interval& a, const interval& b)
		: min(fmin(a.min, b.min)), max(fmax(a.max, b.max)) {}

	bool contains(double x) const {
		return min <= x && x <= max;
	}

	bool surrounds(double x) const {
		return min < x && x < max;
	}

	interval expand(double delta) const {
		auto padding = delta / 2;
		return interval(min - padding, max + padding);
	}

	double clamp(double x) const {
		if (x < min) return min;
		if (x > max) return max;
		return x;
	}
	static const interval empty, universe;
};

const static interval empty(+infinity, -infinity);
const static interval universe(-infinity, +infinity);

#endif