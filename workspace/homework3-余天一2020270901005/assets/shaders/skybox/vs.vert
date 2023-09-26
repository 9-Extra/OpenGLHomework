#version 440 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix;
};

out VS_OUT
{
    vec3 world_pos;
} vs_out;

void main()
{
    vs_out.world_pos = position;
    gl_Position = view_perspective_matrix * vec4(position, 1.0f);
}