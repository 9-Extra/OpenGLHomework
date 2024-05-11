#pragma once

#include "glmath.h"
#include "Material.h"

//---------------------------
// 射线（从屏幕上某点出发追踪的射线），光线
struct Ray		
{
	vec3 start, dir;				// 起点，方向
	Ray(vec3 _start, vec3 _dir) {
		start = _start;

		// 方向要进行归一化，因为在光照计算时，认为参与运算的都是归一化的向量。
		dir = normalize(_dir);		
	}
};

//---------------------------
// 光线和物体表面交点
struct Hit		
{
	float s;					// 距离，直线方程P=P0+su。当t大于0时表示相交，默认取-1表示无交点
    vec2 uv;
	vec3 position, normal;		// 交点坐标，法线
	Material* material;			// 交点处表面的材质
	Hit() { s = -1; }
};
//---------------------------
// 为物品类定义一个基类，表示可求交
class Intersectable			
{
public:
    Intersectable(Material _material): material(_material) {}
	// 虚函数，需要根据不同物品表面类型实现求交
	virtual Hit intersect(const Ray& ray) = 0;		
    virtual ~Intersectable(){}

protected:
	Material material;
};

//---------------------------
// 定义球体
class Sphere : public Intersectable		
{
	vec3 center;
	float radius;
public:
	Sphere(const vec3& _center, float _radius, Material _material): Intersectable(_material), center(_center), radius(_radius) {}
	~Sphere() {}

	// 光线与球体求交点
    Hit intersect(const Ray &ray);
};

//---------------------------
// 平面
class Plane :public Intersectable
{	// 点法式方程表示平面
	vec3 normal;		// 法线
	vec3 p0;			// 线上一点坐标，N(p-p0)=0

public:
	Plane(vec3 _p0, vec3 _normal, Material _material): Intersectable(_material), normal(_normal), p0(_p0) {}

	// 光线与平面求交点
    Hit intersect(const Ray &ray);
};

class Triange :public Intersectable
{	// 点法式方程表示平面
    vec3 v1, v2, v3; //三个顶点
	vec3 normal;// 法线

public:
	Triange(vec3 v1, vec3 v2, vec3 v3, Material _material): Intersectable(_material), v1(v1), v2(v2), v3(v3), normal(normalize(cross(v3 - v2, v1 - v2))) {}

	// 光线与三角形求交点
    Hit intersect(const Ray &ray);
};
