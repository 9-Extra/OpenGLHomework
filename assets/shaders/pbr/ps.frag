#version 440 core

#define POINTLIGNT_MAX 8

layout(binding = 2, std140) uniform per_material
{
    float metallic_factor;
    float roughness_factor;
};

struct PointLight{
    vec3 position; // 0
    vec3 intensity; // 4
};

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix; //16 * 4
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D metallic_roughness_texture;

layout(binding = 5) uniform samplerCube skybox_specular_texture;

in VS_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

#define PI 3.1415926

struct PixelArribute{
    vec3 albedo;
    vec3 normal;
    vec3 emission; // 自发光颜色
    float roughness;
    float metallic;
    vec3 F0;
}; 

float D_GGX(float dotNH, float roughness)
{
    float alpha  = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom  = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r  = (roughness + 1.0);
    float k  = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
float Pow5(float x)
{
    return (x * x * x * x * x);
}

vec3 F_Schlick(float cosTheta, vec3 F0) 
{ 
    return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta); 
    }

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
}

// Specular and diffuse BRDF composition --------------------------------------------
vec3 BRDF(vec3  L,
                vec3  V,
                vec3  N,
                PixelArribute pixel_attribute)
{
    // Precalculate vectors and dot products
    vec3  H     = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    // Light color fixed
    // vec3 lightColor = vec3(1.0);

    vec3 color = vec3(0.0);

    float rroughness = max(0.05, pixel_attribute.roughness);
    // D = Normal distribution (Distribution of the microfacets)
    float D = D_GGX(dotNH, rroughness);
    // G = Geometric shadowing term (Microfacets shadowing)
    float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(dotNV, pixel_attribute.F0);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
    vec3 kD   = (vec3(1.0) - F) * (1.0 - pixel_attribute.metallic);

    color += (kD * pixel_attribute.albedo / PI + (1.0 - kD) * spec);
    
    return color;
}

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
    const vec3 tangent = normalize(vs_out.tangent);
    //在加载时进行了判断所有不需要考虑gamma矫正的问题
    vec3 texture_value = (texture(normal_texture, vs_out.tex_coords).xyz - 0.5) * 2;
    return tangent * texture_value.x + cross(normal, tangent) * texture_value.y + normal * texture_value.z;
}

void main()
{
    const float dielectric_specular = 0.04;//菲涅尔系数
    
    const vec3 N = caculate_normal();//法线
    const vec3 metallic_roughness = texture(metallic_roughness_texture, vs_out.tex_coords).xyz;

    PixelArribute pixel_attribute;
    pixel_attribute.albedo = texture(basecolor_texture, vs_out.tex_coords).xyz;//基础色
    pixel_attribute.normal = N;
    pixel_attribute.roughness = metallic_roughness.y * roughness_factor;//粗糙度
    pixel_attribute.metallic = metallic_roughness.x * metallic_factor;//金属度
    pixel_attribute.F0 = mix(vec3(dielectric_specular), pixel_attribute.albedo, pixel_attribute.metallic);

    vec3 result_color = ambient_light * pixel_attribute.albedo;
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 L = normalize(light_pos - vs_out.world_position);
        vec3 V = normalize(camera_position - vs_out.world_position);

        float squared_distance = dot(light_pos - vs_out.world_position, light_pos - vs_out.world_position);

        vec3 result_light = light_intensity / squared_distance * max(dot(L, N), 0.0f);
        
        result_color += result_light * BRDF(L, V, N, pixel_attribute);
        //result_color = light_intensity;
    }

    result_color = min(result_color, 1.0f);

    const vec3 fog_color = vec3(1.0f ,1.0f ,1.0f);
    float distance = max(0.0f, length(vs_out.world_position - camera_position) - fog_min_distance);
    float fog_factor = exp(-distance * fog_density);
    result_color = mix(fog_color, result_color, fog_factor);

    out_color = vec4(pow(result_color, vec3(1 / 2.2)), 1.0f);
    //out_color = vec4(max(normal,0), 1.0f);
}