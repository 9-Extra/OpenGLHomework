#version 440 core

layout(binding = 2, std140) uniform per_material
{
    vec3 color;
};

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    out_color = vec4(color, 1.0f);
}