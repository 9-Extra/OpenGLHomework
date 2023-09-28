#version 440 core
layout (location = 0) in vec3 position;

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix;
};

layout(binding = 1, std140) uniform per_object
{
    mat4 model_matrix;
};


layout(binding = 2, std140) uniform per_material
{
    vec3 color;
};

void main()
{
    gl_Position = view_perspective_matrix * model_matrix * vec4(position.xyz, 1.0f);
}