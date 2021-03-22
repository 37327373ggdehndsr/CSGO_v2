#include "../utils.h"

namespace math {
	void sin_cos(float rad, float& sin, float& cos) {
		sin = math::sin(rad);
		cos = math::cos(rad);
	}

	void vector_angles(const vec3_t& forward, qangle_t& angles, vec3_t* up) {
		vec3_t  left;
		float   len, up_z, pitch, yaw, roll;

		// get 2d length.
		len = forward.length_2d();

		if (up && len > 0.001f) {
			pitch = rad_to_deg(std::atan2(-forward.z, len));
			yaw = rad_to_deg(std::atan2(forward.y, forward.x));

			// get left direction vector using cross product.
			left = (*up).cross_product(forward).normalized();

			// calculate up_z.
			up_z = (left.y * forward.x) - (left.x * forward.y);

			// calculate roll.
			roll = rad_to_deg(std::atan2(left.z, up_z));
		}

		else {
			if (len > 0.f) {
				// calculate pitch and yaw.
				pitch = rad_to_deg(std::atan2(-forward.z, len));
				yaw = rad_to_deg(std::atan2(forward.y, forward.x));
				roll = 0.f;
			}

			else {
				pitch = (forward.z > 0.f) ? -90.f : 90.f;
				yaw = 0.f;
				roll = 0.f;
			}
		}

		// set out angles.
		angles = { pitch, yaw, roll };
	}

	void angle_vectors(const qangle_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up) {
		vec3_t cos, sin;

		sin_cos(deg_to_rad(angles.x), sin.x, cos.x);
		sin_cos(deg_to_rad(angles.y), sin.y, cos.y);
		sin_cos(deg_to_rad(angles.z), sin.z, cos.z);

		if (forward) {
			forward->x = cos.x * cos.y;
			forward->y = cos.x * sin.y;
			forward->z = -sin.x;
		}

		if (right) {
			right->x = -sin.z * sin.x * cos.y + -cos.z * -sin.y;
			right->y = -sin.z * sin.x * sin.y + -cos.z * cos.y;
			right->z = -sin.z * cos.x;
		}

		if (up) {
			up->x = cos.z * sin.x * cos.y + -sin.z * -sin.y;
			up->y = cos.z * sin.x * sin.y + -sin.z * cos.y;
			up->z = cos.z * cos.x;
		}
	}

	void angle_vectors(const qangle_t& angles, vec3_t& forward) {
		float	sp, sy, cp, cy;

		DirectX::XMScalarSinCos(&sp, &cp, math::deg_to_rad(angles.x));
		DirectX::XMScalarSinCos(&sy, &cy, math::deg_to_rad(angles.y));

		forward.x = cp * cy;
		forward.y = cp * sy;
		forward.z = -sp;
	}

	qangle_t calc_angle(const vec3_t& src, const vec3_t& dst) {
		const auto delta = src - dst;
		if (delta.empty())
			return qangle_t();

		const auto length = delta.length();

		if (delta.z == 0.f && length == 0.f
			|| delta.y == 0.f && delta.x == 0.f)
			return qangle_t();

		auto angles = qangle_t(math::asin(delta.z / length) * math::m_rad_pi, math::atan(delta.y / delta.x) * math::m_rad_pi, 0.f);

		if (delta.x >= 0.f) {
			angles.y += 180.f;
		}

		return angles.normalized();
	}

	float get_fov(const qangle_t& view_angles, const vec3_t& start, const vec3_t& end) {
		vec3_t dir, fw;

		// get direction and normalize.
		dir = (end - start).normalized();

		// get the forward direction vector of the view angles.
		angle_vectors(view_angles, &fw);

		// get the angle between the view angles forward directional vector and the target location.
		return std::max(math::deg_to_rad(std::acos(fw.dot_product(dir))), 0.f);
	}

	void vector_transform(const vec3_t& in1, const matrix3x4_t& in2, vec3_t& out) {
		auto DotProduct = [](const vec3_t& a, const vec3_t& b) {
			return (a.x * b.x + a.y * b.y + a.z * b.z);
		};

		out.x = DotProduct(in1, vec3_t(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3];
		out.y = DotProduct(in1, vec3_t(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3];
		out.z = DotProduct(in1, vec3_t(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3];
	}

	qangle_t interpolate(const qangle_t from, const qangle_t to, const float percent) {
		return to * percent + from * (1.f - percent);
	}

	float interpolate(const float from, const float to, const float percent) {
		return to * percent + from * (1.f - percent);
	}
}
