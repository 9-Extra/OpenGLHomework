#version 440 core

#define POINTLIGNT_MAX 8

layout(binding = 2, std140) uniform per_material
{
    vec3 diffusion_color;
    vec3 ambient_color;
    vec3 emissive_color;
    float specular;
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

in VS_OUT
{
    vec3 normal;
    vec3 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
    return normal;
}

void main()
{    
    const vec3 N = caculate_normal();//法线

    vec3 result_color = emissive_color + ambient_color * diffusion_color;
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 L = normalize(light_pos - vs_out.world_position);
        vec3 V = normalize(camera_position - vs_out.world_position);
        vec3 H = normalize(L + V);

        float squared_distance = dot(light_pos - vs_out.world_position, light_pos - vs_out.world_position);

        vec3 result_light = light_intensity / squared_distance * max(dot(L, N), 0.0f);
        
        vec3 diffu = result_light * diffusion_color;
        vec3 specu = result_light * specular * pow(max(dot(N, H), 0.0f), 32);
        
        result_color += diffu + specu;
    }

    result_color = min(result_color, 1.0f);

    out_color = vec4(result_color, 1.0f);
}