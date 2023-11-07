#include "SThreadPool.h"
#include "scene.h"

static SThreadPool::ThreadPool pool;

Scene scene;

void Scene::render(vector<vec4> &image) {
    //std::cout << "Start Rendering" << std::endl;
    long timeStart = glutGet(GLUT_ELAPSED_TIME);
    // 对视窗的每一个像素做渲染
    for (uint32_t Y = 0; Y < windowHeight; Y++) {
        pool.add_task([this, &image, Y] {
            for (uint32_t X = 0; X < windowWidth; X++) {
                // 追踪这条光线，获得返回的颜色
                vec3 color = trace(viewPoint.getRay(X, Y));
                image[Y * windowWidth + X] = vec4(color.x, color.y, color.z, 1);
            }
        });
        // for (int X = 0; X < windowWidth; X++)
        // {
        // 	// 追踪这条光线，获得返回的颜色
        // 	vec3 color = trace(viewPoint.getRay(X, Y));
        // 	image[Y * windowWidth + X] = vec4(color.x, color.y, color.z, 1);
        // }
    }
    pool.wait_for_all_done();

    cout << "FPS:" << 1.0 / ((glutGet(GLUT_ELAPSED_TIME) - timeStart) * 0.001f) << endl;
}

vec3 Scene::trace(Ray ray, int depth) {
    // 设置迭代终止条件
    if (depth > 5) // 设置迭代上限5次
        return La;
    Hit hit = firstIntersect(ray);

    if (hit.s < 0) // 不再有交，则返回环境光即可
        return La;

    // 针对粗糙材质，计算漫反射
    if (hit.material->type == ROUGH || hit.material->type == ROUGH_TEXTURE) {
        // 初始化返回光线（或者阴影）
        vec3 kd = hit.material->type == ROUGH ? hit.material->kd : sample_image(hit.material->texture, hit.uv);
        vec3 outRadiance = kd * La;
        for (auto &light : lights) {
            vec3 light_vec = hit.position - light->position;
            vec3 light_direction = normalize(light_vec);
            Ray shadowRay(hit.position + hit.normal * epsilon, light_direction);
            float cosTheta = dot(hit.normal, light_direction);
            // 如果cos小于0（钝角），说明光照到的是物体背面，用户看不到
            if (cosTheta > 0) {
                // 如果与其他物体有交，则处于阴影中；反之按Phong模型计算
                if (!shadowIntersect(shadowRay)) {
                    outRadiance = outRadiance + light->Le * kd * cosTheta;
                    vec3 halfway = normalize(-ray.dir + light_direction);
                    float cosDelta = dot(hit.normal, halfway);
                    if (cosDelta > 0)
                        outRadiance =
                            outRadiance + light->Le * hit.material->ks * powf(cosDelta, hit.material->shininess);
                }
            }
        }
        return outRadiance;
    }

    // 镜面反射（继续追踪）
    float cosa = -dot(ray.dir, hit.normal);
    vec3 one(1, 1, 1);
    vec3 F = hit.material->F0 + (one - hit.material->F0) * pow(1 - cosa, 5);
    vec3 reflectedDir = ray.dir - hit.normal * dot(hit.normal, ray.dir) * 2.0f; // 反射光线R = v + 2Ncosa

    // 递归调用trace()
    vec3 outRadiance = trace(Ray(hit.position + hit.normal * epsilon, reflectedDir), depth + 1) * F;

    // 对于透明物体，计算折射（继续追踪）
    if (hit.material->type == REFRACTIVE) {
        float disc = 1 - (1 - cosa * cosa) / hit.material->ior / hit.material->ior;
        if (disc >= 0) {
            vec3 refractedDir = ray.dir / hit.material->ior + hit.normal * (cosa / hit.material->ior - sqrt(disc));

            // 递归调用trace()
            outRadiance =
                outRadiance + trace(Ray(hit.position - hit.normal * epsilon, refractedDir), depth + 1) * (one - F);
        }
    }

    return outRadiance;
}

void Scene::add_cquad(vec3 a, vec3 b, vec3 c, vec3 d, Material mat){
    objects.emplace_back(new Triange(a, b, c, mat));
    objects.emplace_back(new Triange(c, d, a, mat));
}

void Scene::build() {
    vec3 eye = vec3(0, 0, 6), vup = vec3(0, 1, 0), lookat = vec3(0, 0, 0);
    float fov = 45 * M_PI / 180;
    // 视点变换
    viewPoint.set(eye, lookat, vup, fov);
    // 环境光
    La = vec3(0.2f, 0.2f, 0.2f);
    vec3 Le(2, 2, 2); // 光照强度
    lights.emplace_back(std::make_unique<Light>(vec3(-1, 0, -1), Le));

    // 镜面反射率
    vec3 ks(2, 2, 2);

    // 三个球
    objects.emplace_back(new Sphere(vec3(-2.0f, -1.0f, 2.0f), 0.5f, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));
    objects.emplace_back(new Sphere(vec3(0.0f, -1.5f, 2.0f), 0.5f, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));
    objects.emplace_back(new Sphere(vec3(2.0f, -1.5f, -2.0f), 0.5f, Material::RoughMaterial(vec3(1.0f, 1.0f, 1.0f), ks, 20)));

    mat4 R = RotationMatrix(22.5f, vec3(0.0f, 1.0f, 0.0f));
	vec3 V = vec3(2.0f, 0.0f, 2.0f);
    Material mat = Material::TextureMaterial("cube.jpg", ks, 100);
    add_cquad(R * vec3(-0.5f, -2.0f,  0.5f) + V, R * vec3( 0.5f, -2.0f,  0.5f) + V, R * vec3( 0.5f, -1.0f,  0.5f) + V, R * vec3(-0.5f, -1.0f,  0.5f) + V, mat);
    add_cquad(R * vec3( 0.5f, -2.0f, -0.5f) + V, R * vec3(-0.5f, -2.0f, -0.5f) + V, R * vec3(-0.5f, -1.0f, -0.5f) + V, R * vec3( 0.5f, -1.0f, -0.5f) + V, mat);
	add_cquad(R * vec3( 0.5f, -2.0f,  0.5f) + V, R * vec3( 0.5f, -2.0f, -0.5f) + V, R * vec3( 0.5f, -1.0f, -0.5f) + V, R * vec3( 0.5f, -1.0f,  0.5f) + V, mat);
	add_cquad(R * vec3(-0.5f, -2.0f, -0.5f) + V, R * vec3(-0.5f, -2.0f,  0.5f) + V, R * vec3(-0.5f, -1.0f,  0.5f) + V, R * vec3(-0.5f, -1.0f, -0.5f) + V, mat);
	add_cquad(R * vec3(-0.5f, -1.0f,  0.5f) + V, R * vec3( 0.5f, -1.0f,  0.5f) + V, R * vec3( 0.5f, -1.0f, -0.5f) + V, R * vec3(-0.5f, -1.0f, -0.5f) + V, mat);
	add_cquad(R * vec3(-0.5f, -2.0f, -0.5f) + V, R * vec3( 0.5f, -2.0f, -0.5f) + V, R * vec3( 0.5f, -2.0f,  0.5f) + V, R * vec3(-0.5f, -2.0f,  0.5f) + V, mat);
		
    //objects.emplace_back(new Triange(vec3( -50, -2,  50), vec3( 0, -2,  50), vec3( 0, -2,  0), Material::RoughMaterial(vec3(0.0f, 0.5f, 1.0f), ks, 100)));
    
    add_cquad(vec3(-0.0f, -2.0f,  4.0f), vec3( 4.0f, -2.0f,  4.0f), vec3( 4.0f, -2.0f, -4.0f), vec3(-0.0f, -2.0f, -4.0f), Material::TextureMaterial("floor.jpg", ks, 50));
	add_cquad(vec3(-4.0f, -2.0f,  4.0f), vec3( 0.0f, -2.0f,  4.0f), vec3( 0.0f, -2.0f,  0.0f), vec3(-4.0f, -2.0f,  0.0f), Material::TextureMaterial("floor.jpg", ks, 50));
	
    Material white = Material::RoughMaterial(vec3(1.0f, 1.0f, 1.0f), ks, 0);
    //add_cquad(vec3( 0.0f,  2.0f, -4.0f), vec3( 4.0f,  2.0f, -4.0f), vec3( 4.0f,  2.0f,  4.0f), vec3( 0.0f,  2.0f,  4.0f),white);
	add_cquad(vec3(-4.0f,  2.0f,  0.0f), vec3( 0.0f,  2.0f,  0.0f), vec3( 0.0f,  2.0f,  4.0f), vec3(-4.0f,  2.0f,  4.0f),white);
	add_cquad(vec3(-0.0f, -2.0f, -4.0f), vec3( 4.0f, -2.0f, -4.0f), vec3( 4.0f,  2.0f, -4.0f), vec3(-0.0f,  2.0f, -4.0f),white);
	//add_cquad(vec3( 4.0f, -2.0f,  4.0f), vec3(-4.0f, -2.0f,  4.0f), vec3(-4.0f,  2.0f,  4.0f), vec3( 4.0f,  2.0f,  4.0f),white);
	add_cquad(vec3( 4.0f, -2.0f, -4.0f), vec3( 4.0f, -2.0f,  4.0f), vec3( 4.0f,  2.0f,  4.0f), vec3( 4.0f,  2.0f, -4.0f),  Material::RoughMaterial(vec3(0.0f, 1.0f, 0.0f), ks, 0));
	add_cquad(vec3(-4.0f, -2.0f,  4.0f), vec3(-4.0f, -2.0f, -0.0f), vec3(-4.0f,  2.0f, -0.0f), vec3(-4.0f,  2.0f,  4.0f),  Material::RoughMaterial(vec3(1.0f, 0.0f, 0.0f), ks, 0));
	add_cquad(vec3(-4.0f, -2.0f,  0.0f), vec3( 0.0f, -2.0f,  0.0f), vec3( 0.0f,  2.0f,  0.0f), vec3(-4.0f,  2.0f,  0.0f),white);
	add_cquad(vec3( 0.0f, -2.0f,  0.0f), vec3( 0.0f, -2.0f, -4.0f), vec3( 0.0f,  2.0f, -4.0f), vec3( 0.0f,  2.0f,  0.0f),white);


    
    // // 添加物品
    // // 漫反射球
    // // 偏黄色
    // objects.emplace_back(new Sphere(vec3(0.55, -0.2, 0), 0.3, Material::RoughMaterial(vec3(0.3, 0.2, 0.1), ks, 50)));
    // // 偏蓝色
    // objects.emplace_back(new Sphere(vec3(1.2, 0.8, 0), 0.2, Material::RoughMaterial(vec3(0.1, 0.2, 0.3), ks, 100)));
    // // 偏红色
    // objects.emplace_back(new Sphere(vec3(-1.25, 0.8, -0.8), 0.3, Material::RoughMaterial(vec3(0.3, 0, 0.2), ks, 20)));
    // objects.emplace_back(new Sphere(vec3(0, 0.5, -0.8), 0.4, Material::RoughMaterial(vec3(0.3, 0, 0.2), ks, 20)));

    // // 镜面反射球
    // objects.emplace_back(
    //     new Sphere(vec3(-1.2, 0, 0), 0.5, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));
    // objects.emplace_back(
    //     new Sphere(vec3(0.5, 0.5, 0), 0.3, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));

    // 镜面反射平面
    // objects.push_back(new Plane(vec3(0, -0.6,0), vec3(0, 1, 0), new ReflectiveMaterial(vec3(0.14, 0.16, 0.13),
    // vec3(4.1, 2.3, 3.1))));
    // objects.emplace_back(
    //     new Plane(vec3(0, -0.6, 0), vec3(0, 1, 0), Material::RoughMaterial(vec3(0.1, 0.2, 0.3), ks, 100)));
}
