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

in VS_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

#define PI 3.1415926

vec3 ImportanceSampleGGX( vec2 Xi, float Roughness , vec3 N )
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    vec3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize( cross( UpVector , N ) );
    vec3 TangentY = cross( N, TangentX );
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

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
                vec3  F0,
                vec3  basecolor,
                float metallic,
                float roughness)
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

    float rroughness = max(0.05, roughness);
    // D = Normal distribution (Distribution of the microfacets)
    float D = D_GGX(dotNH, rroughness);
    // G = Geometric shadowing term (Microfacets shadowing)
    float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(dotNV, F0);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
    vec3 kD   = (vec3(1.0) - F) * (1.0 - metallic);

    color += (kD * basecolor / PI + (1.0 - kD) * spec);
    // color += (kD * basecolor / PI + spec) * dotNL;
    // color += (kD * basecolor / PI + spec) * dotNL * lightColor;

    return color;
}

vec3 pbr(vec3 viewer_pos, vec3 world_pos, vec3 light_pos, vec3 light_intensity, vec3 basecolor, float metallic, float roughness, vec3 normal){
    if (dot(normal, viewer_pos - world_pos) <= 0.0f || dot(normal, light_pos - world_pos) <= 0.0f){
        //光照到背面或者相机在背面时返回0
        return vec3(0.0f, 0.0f ,0.0f);
    }

    const float dielectric_specular = 0.04;//菲涅尔系数

    // float cos_theta = dot(normal, normalize(viewer_pos - world_pos));//法线与观察角的cos

    // vec3 term1 = (1 - metallic_factor) * basecolor / 3.1415926f;
    // float F = F0 + (1 - F0) * (1 - cos_theta);
    vec3 L = normalize(light_pos - world_pos);
    vec3 V = normalize(viewer_pos - world_pos);
    vec3 N = normal;
    vec3 F0 = mix(vec3(dielectric_specular, dielectric_specular, dielectric_specular), basecolor, metallic);

    return BRDF(L, V, N, F0, basecolor, metallic, roughness);
}

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
    const vec3 tangent = normalize(vs_out.tangent);
    //需要反向gamma矫正以获取正确的数值
    vec3 texture_value = (texture(normal_texture, vs_out.tex_coords).xyz - 0.5) * 2;
    return tangent * texture_value.x + cross(normal, tangent) * texture_value.y + normal * texture_value.z;
}

void main()
{
    const vec3 normal = caculate_normal();
    const vec3 basecolor = texture(basecolor_texture, vs_out.tex_coords).xyz;
    const vec3 metallic_roughness = texture(metallic_roughness_texture, vs_out.tex_coords).xyz;
    float metallic = metallic_roughness.x * metallic_factor;
    float roughness = metallic_roughness.y * roughness_factor;

    vec3 result_color = ambient_light * basecolor;
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 light_vec = light_pos - vs_out.world_position;
        vec3 oritation = normalize(light_vec);

        vec3 result_light = light_intensity / dot(light_vec, light_vec) * max(dot(oritation, normal), 0.0f);
        
        result_color += result_light * pbr(camera_position, vs_out.world_position, light_pos, light_intensity, basecolor, metallic, roughness, normal);
        //result_color = light_intensity;
    }

    result_color = min(result_color, 1.0f);

    const vec3 fog_color = vec3(1.0f ,1.0f ,1.0f);
    float distance = max(0.0f, length(vs_out.world_position - camera_position) - fog_min_distance);
    float fog_factor = exp(-distance * fog_density);
    result_color = mix(fog_color, result_color, fog_factor);
    // if (pointlight_num == 0){
    //     result_color = vec3(1.0f, 0.0f, 0.0f);
    // } else if (pointlight_num == 1){
    //     result_color = vec3(0.0f, 1.0f, 0.0f);
    // }

    out_color = vec4(pow(result_color, vec3(1 / 2.2)), 1.0f);
    //out_color = vec4(max(normal,0), 1.0f);
}