#include <iostream>
#include <cmath>
#include <assert.h>

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    Vector3f() {}
    Vector3f(float x, float y, float z): x(x), y(y), z(z) {}

    const float *data() const { return (float *)v; }

    inline float operator[](const unsigned int i) const { return v[i]; }

    inline Vector3f operator+(const Vector3f b) const {
        return Vector3f{x + b.x, y + b.y, z + b.z};
    }
    inline Vector3f operator+(float b) const {
        return Vector3f{x + b, y + b, z + b};
    }

    inline Vector3f operator-(const Vector3f b) const {
        return Vector3f{x - b.x, y - b.y, z - b.z};
    }

    inline Vector3f operator-() const {
        return Vector3f{-x, -y, -z};
    }

    inline Vector3f operator+=(const Vector3f b) { return *this = *this + b; }

    inline Vector3f operator*(const float n) { return {x * n, y * n, z * n}; }
    inline Vector3f operator/(const float n) { return *this * (1.0f / n);}

    inline float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    inline Vector3f cross(const Vector3f b) {
        return {this->y * b.z - this->z * b.y, this->z * b.x * this->x * b.z,
                this->x * b.y - this->y * b.x};
    }

    inline float square() const { return this->dot(*this); }

    inline float length() const {
        return std::sqrt(square());
    }

    friend std::ostream& operator << (std::ostream& os, const Vector3f& v)
	{
		os << "(" << v.x << ", " << v.y << ", " << v.z << ")"; 
		return os;
	}
};

Vector3f rgb2hsv(Vector3f rgb_color){
    auto [r, g, b] = rgb_color.v;
    float c_max = std::max(r, std::max(g, b));
    float c_min = std::min(r, std::min(g, b));
    float delta = c_max - c_min;

    float hue;
    if (delta == 0.0f){
        hue = 0.0f;
    } else if (r > g && r > b){
        hue = 60.0f * fmod((g - b) / delta, 6);
        if (hue < 0.0f) hue += 360.0f;
    } else if (g > r && g > b){
        hue = 60.0f * ((b - r) / delta + 2.0f);
    } else {
        hue = 60.0f * ((r - g) / delta + 4.0f);
    }

    float saturation = c_max == 0.0f ? 0.0f : delta / c_max;
    
    float value = c_max;

    return {hue, saturation, value};
}

Vector3f hsv2rgb(Vector3f hsv_color){
    auto [h, s, v] = hsv_color.v;
    assert(h >= 0.0f && h <= 360.0f);

    float c = v * s;
    float x = c * (1.0f - std::abs(fmod(h / 60.0f, 2) - 1.0f));
    float m = v - c;

    switch (((unsigned int)h) / 60) {
        case 0:{
            return Vector3f(c, x, 0.0f) + m;
        }
        case 1:{
            return Vector3f(x, c, 0.0f) + m;
        }
        case 2:{
            return Vector3f(0.0f, c, x) + m;
        }
        case 3:{
            return Vector3f(0.0f, x, c) + m;
        }
        case 4:{
            return Vector3f(x, 0.0f, c) + m;
        }
        case 5:
        case 6:
        {
            return Vector3f(c, 0.0f, x) + m;
        }
    }
    //不应该到达这里
    return Vector3f();
}

int main(){
    Vector3f color{0.5f, 0.2f, 0.3f};
    Vector3f hsv = rgb2hsv(color);
    std::cout << "初始：" << color << std::endl;
    std::cout << "转化为HSV：" << hsv << std::endl;
    Vector3f p = hsv2rgb(hsv);

    std::cout << "再转化回来：" << p << std::endl;

    return 0;
}