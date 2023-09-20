#version 440 core

#define POINTLIGNT_MAX 8

layout(binding = 2, std140) uniform per_material
{
    vec3 color;
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
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 3) uniform sampler2D diffision_texture;
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
    const vec3 diffision = texture(diffision_texture, vs_out.tex_coords).xyz;

    vec3 result_color = ambient_light * diffision;
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        vec3 light_vec = light_pos - vs_out.world_position;
        vec3 oritation = normalize(light_vec);

        vec3 result_light = light_intensity / dot(light_vec, light_vec) * max(dot(oritation, normal), 0.0f);
        
        result_color += result_light * diffision;
        //result_color = light_intensity;
    }

    result_color = min(result_color, 1.0f);
    // if (pointlight_num == 0){
    //     result_color = vec3(1.0f, 0.0f, 0.0f);
    // } else if (pointlight_num == 1){
    //     result_color = vec3(0.0f, 1.0f, 0.0f);
    // }

    out_color = vec4(result_color, 1.0f);
}