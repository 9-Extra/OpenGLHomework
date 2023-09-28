#version 440 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

#define POINTLIGNT_MAX 8

struct PointLight{
    vec3 position;
    vec3 intensity;
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

layout(binding = 1, std140) uniform per_object
{
    mat4 model_matrix;
    mat4 normal_matrix;
};

out VS_OUT
{
    vec3 normal;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

void main()
{
    vec4 world_position = model_matrix * vec4(position.xyz, 1.0f);
    vs_out.world_position = world_position.xyz / world_position.w;
    vs_out.normal = (normal_matrix * vec4(normal, 0.0f)).xyz;
    vs_out.tex_coords = uv;

    gl_Position = view_perspective_matrix * world_position;
}