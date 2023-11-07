#pragma once
#include <GL/glew.h>		
#include <GL/glut.h>	
#include "Intersectable.h"

#include <vector>
#include <memory>
#include "global.h"

//---------------------------
// 定义光源
struct DirectionalLight {
    vec3 direction;			
	vec3 Le;			// 光照强度
	DirectionalLight(vec3 direction, vec3 _Le): direction(direction), Le(_Le) {}
};

struct PointLight {
    vec3 position;			
	vec3 Le;			// 光照强度
	PointLight(vec3 position, vec3 _Le): position(position), Le(_Le) {}
};

//---------------------------
// 视点类
class ViewPoint {		// 表示用户视线
	// eye用来定义用户位置，即视点；lookat(视线中心)，right和up共同定义了视窗大小
	vec3 eye, lookat, right, up;		
	float fov;

public:
	// 获得屏幕上某点的光线
	Ray getRay(int X, int Y)
	{
		vec3 dir = lookat + right * (2 * (X + 0.5f) / windowWidth - 1) + 
						up * (2 * (Y + 0.5f) / windowHeight - 1) - eye;
						
		return Ray(eye, dir);
	}

	// 设置视点位置、fov视域角等参数
	void set(vec3 _eye, vec3 _lookat, vec3 _up, float _fov)	
	{
		eye = _eye;
		lookat = _lookat;
		fov = _fov;
		vec3 w = eye - lookat;			
		float windowSize = length(w) * tanf(fov / 2);
		// 要确保up、right与eye到lookat的向量垂直(所以叉乘)
		right = normalize(cross(_up, w)) * windowSize;	
		up = normalize(cross(w, right)) * windowSize;
	}


	void animate(float dt)		// 修改eye的位置（旋转）
	{
		vec3 d = eye - lookat;
		eye = vec3(d.x * cos(dt) + d.z * sin(dt), d.y, -d.x * sin(dt) + d.z * cos(dt)) + lookat;
		set(eye, lookat, up, fov);
	}

	void zoomInOut(float dz)		// 修改eye的位置
	{
		eye = eye + normalize(eye - lookat) * dz;
		set(eye, lookat, up, fov);
		//cout << "\t" << eye.z << " " << dz << endl;
	
	}
};
//---------------------------
// 场景，物品和光源集合
class Scene {
	vector<std::unique_ptr<Intersectable>> objects; // 物品
	// 光源
	vector<DirectionalLight> direction_lights;
    vector<PointLight> point_lights;
	ViewPoint viewPoint;
	vec3 La;		// 环境光
public:
	// 初始化函数，定义了用户(摄像机)的初始位置，环境光La，方向光源集合、物品集合中添加物件
    void build();

    void add_cquad(vec3 a, vec3 b, vec3 c, vec3 d, Material mat);

    // 渲染视窗上每个点的着色(逐像素调用trace函数)
    void render(vector<vec4> &image);
        // 求最近的交点
	Hit firstIntersect(Ray ray)		
	{
		Hit bestHit;
		for (auto& object : objects)
		{
			Hit hit = object->intersect(ray);
			if (hit.s > 0 && (bestHit.s < 0 || hit.s < bestHit.s))
				bestHit = hit;
		}

		// 光线与交点的点积大于0，夹角为锐角
		if (dot(ray.dir, bestHit.normal) > 0)
			bestHit.normal = bestHit.normal * (-1);

		return bestHit;
	}
	// 该射线在指向光源的路径上是否与其他物体有交
	bool shadowIntersect(Ray ray)	
	{
		for (auto& object : objects)
			if (object->intersect(ray).s > 0)
				return true;
		return false;
	}
	// 光线追踪算法主体代码
    vec3 trace(Ray ray, int depth = 0);

    vec3 phong_shading(vec3 V, const Hit& hit){
        vec3 kd = hit.material->type == ROUGH ? hit.material->kd : sample_image(hit.material->texture, hit.uv);
        
        // 环境光
        vec3 outRadiance = kd * La;

        // 方向光
        for (const auto &light : direction_lights) {
            vec3 L = light.direction;
            Ray shadowRay(hit.position + hit.normal * epsilon, L);
            float cosTheta = dot(hit.normal, L);
            // 如果cos小于0（钝角），说明光照到的是物体背面，用户看不到
            if (cosTheta > 0) {
                // 如果与其他物体有交，则处于阴影中；反之按Phong模型计算
                if (!shadowIntersect(shadowRay)) {
                    // 漫反射
                    outRadiance += light.Le * kd * cosTheta;
                    // 高光
                    vec3 H = normalize(V + L);
                    float cosDelta = dot(hit.normal, H);
                    if (cosDelta > 0)
                        outRadiance += light.Le * hit.material->ks * powf(cosDelta, hit.material->shininess);
                }
            }
        }

        // 点光源
        for (const auto &light : point_lights) {
            vec3 light_direction = light.position - hit.position;
            vec3 L = normalize(light_direction);
            Ray shadowRay(hit.position + hit.normal * epsilon, L);
            float cosTheta = dot(hit.normal, L);
            // 如果cos小于0（钝角），说明光照到的是物体背面，用户看不到
            if (cosTheta > 0) {
                // 如果与其他物体有交，则处于阴影中；反之按Phong模型计算
                if (!shadowIntersect(shadowRay)) {
                    // 漫反射
                    float squared_distance = length(light_direction); // 使用线性光衰
                    outRadiance += light.Le * kd * cosTheta / squared_distance;
                    // 高光
                    vec3 H = normalize(V + L);
                    float cosDelta = dot(hit.normal, H);
                    if (cosDelta > 0)
                        outRadiance += light.Le * hit.material->ks * powf(cosDelta, hit.material->shininess) / squared_distance;
                }
            }
        }
        
        return outRadiance;
    }

    // 动画，相机绕场景旋转
	void animate(float dt)
	{
		viewPoint.animate(dt);
	}

	// 视点变化
	void zoomInOut(float dz)
	{
		viewPoint.zoomInOut(dz);
	}
};

// 场景对象 
extern Scene scene;