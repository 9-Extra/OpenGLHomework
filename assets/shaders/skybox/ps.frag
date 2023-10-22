#version 440 core

layout(binding = 5) uniform samplerCube skybox_specular_texture;

in VS_OUT
{
    vec3 world_pos;
} fs_in;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    vec3 result_color = texture(skybox_specular_texture, fs_in.world_pos).xyz;
    //vec3 result_color = texture(skybox_color_texture, vec3(0, -1, 0)).xyz;
    //vec3 result_color = abs(fs_in.world_pos);
    //vec3 result_color = normalize(abs(vec3(0, 0, 1)));
    out_color = vec4(pow(result_color, vec3(1 / 2.2)), 1.0f);
}