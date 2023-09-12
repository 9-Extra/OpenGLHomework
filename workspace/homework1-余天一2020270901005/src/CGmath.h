#pragma once

#include <cmath>

static float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;                       // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);               // what the fuck?
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));   // 1st iteration
	//y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

inline float to_radian(float angle) {
	return angle / 180.0f * 3.1415926535f;
}

struct Vector2f {
	union {
		struct
		{
			float x, y;
		};
		float v[2];
	};
	Vector2f() : x(0), y(0) {}
	Vector2f(float x, float y) : x(x), y(y) {}

	Vector2f operator+(const Vector2f b) {
		return Vector2f(x + b.x, y + b.y);
	}

	Vector2f operator-(const Vector2f b) {
		return Vector2f(x - b.x, y - b.y);
	}

	Vector2f operator*(const float s) {
		return Vector2f(x * s, y * s);
	}

	float squared() {
		return x * x + y * y;
	}

	float length() {
		return sqrtf(squared());
	}

	Vector2f normalized() {
		float s = Q_rsqrt(squared());
		return Vector2f(x * s, y * s);
	}

	Vector2f rotate(float radiam) {
		return Vector2f(x * cosf(radiam) + y * sinf(radiam), x * -sinf(radiam) + y * cosf(radiam));
	}
};

// struct Quaternion {
// 	float x, y, z, w;

//     static constexpr Quaternion no_rotate(){
//         return Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
//     }

// 	//需要axis长度为1
// 	static Quaternion from_rotation(Vector3f axis, float angle) {
// 		angle = angle * 0.5f;
// 		float sin_theta = sinf(angle), cos_theta = cosf(angle);
// 		return Quaternion{ sin_theta * axis.x, sin_theta * axis.y ,sin_theta * axis.z, cos_theta };
// 	}

// 	//按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度
// 	static Quaternion from_eular(Vector3f rotate) {
// 		return
// 			Quaternion::from_rotation({ 1.0f, 0.0f, 0.0f }, rotate.x) *
// 			Quaternion::from_rotation({ 0.0f, 1.0f, 0.0f }, rotate.y) *
// 			Quaternion::from_rotation({ 0.0f, 0.0f, 1.0f }, rotate.z);
	
// 	}

// 	//需要长度为1
// 	Matrix4f to_matrix() const{
// 		return Matrix4f{ {{
// 			1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y), 0.0f,
// 			2.0f * (x * y + w * z),1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x), 0.0f,
// 			2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y), 0.0f,
// 			0.0f,0.0f,0.0f,1.0f
// 		}} };
// 	}

// 	Vector3f rotate_vector(Vector3f src) const{
// 		const Quaternion& q = *this;
// 		Quaternion p{ src.x, src.y, src.z, 1.0f };
// 		Quaternion rotated = q * p * q.conjugate();
// 		return Vector3f{ rotated.x, rotated.y, rotated.z };
// 	}

// 	Quaternion conjugate() const{
// 		return Quaternion{ -x, -y, -z, w };
// 	}

// 	Quaternion operator* (const Quaternion r) const{
// 		Vector3f qv{ x, y, z }, rv{ r.x, r.y, r.z };
// 		Vector3f v = qv.cross(rv) + qv * r.w + rv * w;

// 		return Quaternion{ v.x, v.y, v.z, w * r.w - qv.dot(rv) };
// 	}
// };