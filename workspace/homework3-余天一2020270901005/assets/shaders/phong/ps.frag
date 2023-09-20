#version 440 core

#define POINTLIGNT_MAX 8

layout(binding = 2, std140) uniform per_material
{
    vec3 color;
};

struct PointLight{
    vec3 position;
    vec3 intensity;
};

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix;
    vec3 ambient_light;
    uint pointlight_num;
    PointLight pointlight_list[POINTLIGNT_MAX];
};

// layout(binding = 3) uniform sampler2D diffision_texture;
// layout(binding = 4) uniform sampler2D normal_texture;
// layout(binding = 5) uniform sampler2D specular_texture;

in VS_OUT
{
    vec3 normal;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    const vec3 normal = normalize(vs_out.normal);
    out_color = vec4(abs(normal), 1.0f);
}