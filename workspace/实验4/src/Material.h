#pragma once

#include "glmath.h"
#include <FreeImage.h>
#include <string>

FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path);

// 材质定义
// 粗糙(Rough)、反射型(Reflective)、折射型/透明物体(Refractive/Transparent)
enum MaterialType { ROUGH, ROUGH_TEXTURE, REFLECTIVE, REFRACTIVE };

inline vec3 sample_image(FIBITMAP* image, vec2 uv){
    unsigned int w = FreeImage_GetWidth(image);
    unsigned int h = FreeImage_GetHeight(image);
    RGBQUAD rgb;
    FreeImage_GetPixelColor(image, w * uv[0], h * uv[1], &rgb);

    return vec3(rgb.rgbRed / 255.0f, rgb.rgbGreen / 255.0f, rgb.rgbBlue / 255.0f);
}
//---------------------------
// 材质基类
struct Material
{
	// 环境光照(Ambient)，漫反射(Diffuse)，镜面反射(Specular)系数，用于phong模型计算
	vec3 ka, kd, ks;
	// 镜面反射强度（或者理解为物体表面的平整程度）
	float shininess;
	// 反射比。计算公式需要折射率n和消光系数k：[(n-1)^2+k^2]/[(n+1)^2+k^2]	
	vec3 F0;
	// 折射率索引（index of refraction）			
	float ior;			
    FIBITMAP* texture; // 贴图
	MaterialType type;
	Material(MaterialType t) { type = t; }

    // 粗糙单色材质
    static Material RoughMaterial(vec3 _kd, vec3 _ks, float _shininess){
        Material m(ROUGH);
        m.ka = _kd * M_PI;
		m.kd = _kd;
		m.ks = _ks;
		m.shininess = _shininess;
        return m;
    }

    // 粗糙贴图材质
    static Material TextureMaterial(const std::string &image_path, vec3 _ks, float _shininess){
        Material m(ROUGH_TEXTURE);
        m.texture = freeimage_load_and_convert_image(image_path);
		m.ks = _ks;
		m.shininess = _shininess;
        return m;
    }

    // 派生类：反射材质
    // n: 反射率；
	// kappa：extinction coefficient(消光系数)
    static Material ReflectiveMaterial(vec3 n, vec3 kappa) {
        Material m(REFLECTIVE);
        vec3 one(1, 1, 1);
		m.F0 = ((n - one) * (n - one) + kappa * kappa) / ((n + one) * (n + one) + kappa * kappa);		// Fresnel公式
        return m;
    }

    // n：折射率；一个物体透明，光显然不会在其内部消逝，因此第二项忽略
    static Material RefractiveMaterial(vec3 n) {
        Material m(REFRACTIVE);
        vec3 one(1, 1, 1);
        m.F0 = ((n - one) * (n - one)) / ((n + one) * (n + one));
		m.ior = n.x;		// 该物体的折射率取单色光或取均值都可以
        return m;
    }
};