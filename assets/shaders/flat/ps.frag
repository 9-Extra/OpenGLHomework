#version 440 core

layout(binding = 2, std140) uniform per_material
{
    vec3 base_color;
};


layout(binding = 3) uniform sampler2D color_texture;

in VS_OUT
{
    vec2 tex_coords;
} fs_in;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    out_color = texture(color_texture, fs_in.tex_coords) * vec4(base_color, 1.0);
}