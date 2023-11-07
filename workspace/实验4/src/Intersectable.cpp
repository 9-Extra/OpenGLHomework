#include "Intersectable.h"

Hit Sphere::intersect(const Ray &ray) {
    Hit hit;
    vec3 dist = ray.start - center;  // 距离
    float a = dot(ray.dir, ray.dir); // dot表示点乘，这里是联立光线与球面方程
    float b = dot(dist, ray.dir) * 2.0f;
    float c = dot(dist, dist) - radius * radius;
    float discr = b * b - 4.0f * a * c; // b^2-4ac
    if (discr < 0)                      // 无交点
        return hit;
    float sqrt_discr = sqrtf(discr);
    float s1 = (-b + sqrt_discr) / 2.0f / a; // 求得两个交点，s1 >= s2
    float s2 = (-b - sqrt_discr) / 2.0f / a;
    if (s1 <= 0)
        return hit;
    hit.s = (s2 > 0) ? s2 : s1; // 取近的那个交点
    hit.position = ray.start + ray.dir * hit.s;
    hit.normal = (hit.position - center) / radius;
    hit.uv = vec2(0, 0); // 不考虑球面uv
    hit.material = &material;
    return hit;
}
Hit Plane::intersect(const Ray &ray) {
    Hit hit;
    // 射线方向与法向量点乘，为0表示平行
    float nD = dot(ray.dir, normal);
    if (nD == 0)
        return hit;

    // 无效交点
    float s1 = (dot(normal, p0) - dot(normal, ray.start)) / nD;
    if (s1 < 0)
        return hit;

    // 交点
    hit.s = s1;
    hit.position = ray.start + ray.dir * hit.s;
    hit.normal = normal;
    hit.uv = vec2(0, 0); // 不用
    hit.material = &material;

    return hit;
}
Hit Triange::intersect(const Ray &ray) {
    Hit hit;
    // 射线方向与法向量相同，不可见
    float nD = dot(ray.dir, normal);
    if (nD >= 0){
        return hit;
    }

    float distance = dot(v1 - ray.start, normal) / nD;
    if (distance < 0) {
        return hit; // 在背后
    }

    vec3 point = ray.start + ray.dir * distance;
    vec2 uv = solve_uv_guass(v1, v2, v3, point);
    if (uv[0] < 0.0 || uv[1] < 0.0 || uv[0] + uv[1] > 1.0) {
        return hit; // 交点在三角形外
    }

    // 交点
    hit.s = distance;
    hit.position = point;
    hit.normal = normal;
    hit.uv = uv;
    hit.material = &material;

    return hit;
}
