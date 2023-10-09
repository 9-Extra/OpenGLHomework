#version 440 core
layout (location = 0) in vec3 position;

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
    vec4 pos = view_perspective_matrix * vec4(position, 1.0f);
    gl_Position = pos.xyww;// 相当于z = w， 欺骗一下深度测试 https://www.jianshu.com/p/ad691b3ea9d5
}