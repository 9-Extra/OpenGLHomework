#version 440 core
#extension GL_EXT_gpu_shader4 : enable   

layout(binding = 1, std140) uniform per_object
{
    mat4 model_matrix;
    int object_id;
};

out uint out_color; // 输出整数

void main()
{
    out_color = object_id;
}