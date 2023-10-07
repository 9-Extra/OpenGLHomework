#include <GL/glew.h>		
#include <GL/glut.h>	
#include "SThreadPool.h"
#include "glmath.h"
#include <memory>


static SThreadPool::ThreadPool pool;

using namespace std;
// 全局变量
// Resolution of screen
const unsigned int windowWidth = 600, windowHeight = 600;
const float epsilon = 0.0001f;

// 材质定义
// 粗糙(Rough)、反射型(Reflective)、折射型/透明物体(Refractive/Transparent)
enum MaterialType { ROUGH, REFLECTIVE, REFRACTIVE };

//---------------------------
// 材质基类
struct Material
{
	// 环境光照(Ambient)，漫反射(Diffuse)，镜面反射(Specular)系数，用于phong模型计算
	vec3 ka, kd, ks;
	// 镜面反射强度（或者理解为物体表面的平整程度）
	float shininess;
	// 反射比。计算公式需要折射率n和消光系数k：[(n-1)^2+k^2]/[(n+1)^2+k^2]	
	vec3 F0;
	// 折射率索引（index of refraction）			
	float ior;			
	MaterialType type;
	Material(MaterialType t) { type = t; }

    // 粗糙材质
    static Material RoughMaterial(vec3 _kd, vec3 _ks, float _shininess){
        Material m(ROUGH);
        m.ka = _kd * M_PI;
		m.kd = _kd;
		m.ks = _ks;
		m.shininess = _shininess;
        return m;
    }

    // 派生类：反射材质
    // n: 反射率；
	// kappa：extinction coefficient(消光系数)
    static Material ReflectiveMaterial(vec3 n, vec3 kappa) {
        Material m(REFLECTIVE);
        vec3 one(1, 1, 1);
		m.F0 = ((n - one) * (n - one) + kappa * kappa) / ((n + one) * (n + one) + kappa * kappa);		// Fresnel公式
        return m;
    }

    // n：折射率；一个物体透明，光显然不会在其内部消逝，因此第二项忽略
    static Material RefractiveMaterial(vec3 n) {
        Material m(REFRACTIVE);
        vec3 one(1, 1, 1);
        m.F0 = ((n - one) * (n - one)) / ((n + one) * (n + one));
		m.ior = n.x;		// 该物体的折射率取单色光或取均值都可以
        return m;
    }
};

//---------------------------
// 光线和物体表面交点
struct Hit		
{
	float s;					// 直线方程P=P0+su。当t大于0时表示相交，默认取-1表示无交点
	vec3 position, normal;		// 交点坐标，法线
	Material* material;			// 交点处表面的材质
	Hit() { s = -1; }
};

//---------------------------
// 射线（从屏幕上某点出发追踪的射线），光线
struct Ray		
{
	vec3 start, dir;				// 起点，方向
	Ray(vec3 _start, vec3 _dir) {
		start = _start;

		// 方向要进行归一化，因为在光照计算时，认为参与运算的都是归一化的向量。
		dir = normalize(_dir);		
	}
};

//---------------------------
// 为物品类定义一个基类，表示可求交
class Intersectable			
{
public:
    Intersectable(Material _material): material(_material) {}
	// 虚函数，需要根据不同物品表面类型实现求交
	virtual Hit intersect(const Ray& ray) = 0;		

protected:
	Material material;
};

//---------------------------
// 定义球体
class Sphere : public Intersectable		
{
	vec3 center;
	float radius;
public:
	Sphere(const vec3& _center, float _radius, Material _material): Intersectable(_material), center(_center), radius(_radius) {}
	~Sphere() {}

	// 光线与球体求交点
	Hit intersect(const Ray& ray)
	{
		Hit hit;
		vec3 dist = ray.start - center;			// 距离
		float a = dot(ray.dir, ray.dir);		// dot表示点乘，这里是联立光线与球面方程
		float b = dot(dist, ray.dir) * 2.0f;
		float c = dot(dist, dist) - radius * radius;
		float discr = b * b - 4.0f * a * c;		// b^2-4ac
		if (discr < 0)		// 无交点
			return hit;
		float sqrt_discr = sqrtf(discr);
		float s1 = (-b + sqrt_discr) / 2.0f / a;	// 求得两个交点，s1 >= s2
		float s2 = (-b - sqrt_discr) / 2.0f / a;
		if (s1 <= 0)
			return hit;
		hit.s = (s2 > 0) ? s2 : s1;		// 取近的那个交点
		hit.position = ray.start + ray.dir * hit.s;
		hit.normal = (hit.position - center) / radius;
		hit.material = &material;
		return hit;
	}
};

//---------------------------
// 平面
class Plane :public Intersectable
{	// 点法式方程表示平面
	vec3 normal;		// 法线
	vec3 p0;			// 线上一点坐标，N(p-p0)=0

public:
	Plane(vec3 _p0, vec3 _normal, Material _material): Intersectable(_material), normal(_normal), p0(_p0) {}

	// 光线与平面求交点
	Hit intersect(const Ray& ray)
	{
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
		hit.material = &material;

		return hit;
	}
};

//---------------------------
// 定义光源
struct Light {			
	vec3 direction;
	vec3 Le;			// 光照强度
	Light(vec3 _direction, vec3 _Le)
	{
		direction = _direction;
		Le = _Le;
	}
};

//---------------------------
// 视点类
class ViewPoint {		// 表示用户视线
	// eye用来定义用户位置，即视点；lookat(视线中心)，right和up共同定义了视窗大小
	vec3 eye, lookat, right, up;		
	float fov;

public:
	// 获得屏幕上某点的光线
	Ray getRay(int X, int Y)
	{
		vec3 dir = lookat + right * (2 * (X + 0.5f) / windowWidth - 1) + 
						up * (2 * (Y + 0.5f) / windowHeight - 1) - eye;
						
		return Ray(eye, dir);
	}

	// 设置视点位置、fov视域角等参数
	void set(vec3 _eye, vec3 _lookat, vec3 _up, float _fov)	
	{
		eye = _eye;
		lookat = _lookat;
		fov = _fov;
		vec3 w = eye - lookat;			
		float windowSize = length(w) * tanf(fov / 2);
		// 要确保up、right与eye到lookat的向量垂直(所以叉乘)
		right = normalize(cross(_up, w)) * windowSize;	
		up = normalize(cross(w, right)) * windowSize;
	}


	void animate(float dt)		// 修改eye的位置（旋转）
	{
		vec3 d = eye - lookat;
		eye = vec3(d.x * cos(dt) + d.z * sin(dt), d.y, -d.x * sin(dt) + d.z * cos(dt)) + lookat;
		set(eye, lookat, up, fov);
	}

	void zoomInOut(float dz)		// 修改eye的位置
	{
		eye = eye + normalize(eye - lookat) * dz;
		set(eye, lookat, up, fov);
		//cout << "\t" << eye.z << " " << dz << endl;
	
	}
};

//---------------------------
// 场景，物品和光源集合
class Scene {
	vector<std::unique_ptr<Intersectable>> objects; // 物品
	// 光源
	vector<std::unique_ptr<Light>> lights;
	ViewPoint viewPoint;
	vec3 La;		// 环境光
public:
	// 初始化函数，定义了用户(摄像机)的初始位置，环境光La，方向光源集合、物品集合中添加物件
	void build()
	{
		vec3 eye = vec3(0, 0, 6), vup = vec3(0, 1, 0), lookat = vec3(0, 0, 0);
		float fov = 45 * M_PI / 180;
		// 视点变换
		viewPoint.set(eye, lookat, vup, fov);
		// 环境光
		La = vec3(0.4f, 0.4f, 0.4f);
		vec3 lightDirection(1, 1, 1);
		vec3 Le(2, 2, 2); // 光照强度
		lights.emplace_back(std::make_unique<Light>(lightDirection, Le));

		// 镜面反射率
		vec3 ks(2, 2, 2);

		// 添加物品
		// 漫反射球
		// 偏黄色
		objects.emplace_back(new Sphere(vec3(0.55, -0.2, 0), 0.3, Material::RoughMaterial(vec3(0.3, 0.2, 0.1), ks, 50)));  
		// 偏蓝色
		objects.emplace_back(new Sphere(vec3(1.2, 0.8, 0), 0.2, Material::RoughMaterial(vec3(0.1, 0.2, 0.3), ks, 100)));  
		// 偏红色
		objects.emplace_back(new Sphere(vec3(-1.25, 0.8, -0.8), 0.3, Material::RoughMaterial(vec3(0.3, 0, 0.2), ks, 20)));	 
		objects.emplace_back(new Sphere(vec3(0, 0.5, -0.8), 0.4, Material::RoughMaterial(vec3(0.3, 0, 0.2), ks, 20)));	
		
		// 镜面反射球
		objects.emplace_back(new Sphere(vec3(-1.2, 0, 0), 0.5, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));	
		objects.emplace_back(new Sphere(vec3(0.5, 0.5, 0), 0.3, Material::ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));	
		
		// 镜面反射平面
		// objects.push_back(new Plane(vec3(0, -0.6,0), vec3(0, 1, 0), new ReflectiveMaterial(vec3(0.14, 0.16, 0.13), vec3(4.1, 2.3, 3.1))));
		objects.emplace_back(new Plane(vec3(0, -0.6,0), vec3(0, 1, 0), Material::RoughMaterial(vec3(0.1, 0.2, 0.3), ks, 100))); 
	}

    // 渲染视窗上每个点的着色(逐像素调用trace函数)
    void render(vector<vec4> &image) {
        // std::cout << "Start Rendering" << std::endl;
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
        // 求最近的交点
	Hit firstIntersect(Ray ray)		
	{
		Hit bestHit;
		for (auto& object : objects)
		{
			Hit hit = object->intersect(ray);
			if (hit.s > 0 && (bestHit.s < 0 || hit.s < bestHit.s))
				bestHit = hit;
		}

		// 光线与交点的点积大于0，夹角为锐角
		if (dot(ray.dir, bestHit.normal) > 0)
			bestHit.normal = bestHit.normal * (-1);

		return bestHit;
	}
	// 该射线在指向光源的路径上是否与其他物体有交
	bool shadowIntersect(Ray ray)	
	{
		for (auto& object : objects)
			if (object->intersect(ray).s > 0)
				return true;
		return false;
	}
	// 光线追踪算法主体代码
	vec3 trace(Ray ray, int depth = 0)		
	{
		// 设置迭代终止条件
		if (depth > 5)		// 设置迭代上限5次
			return La;
		Hit hit = firstIntersect(ray);

		if (hit.s < 0)		// 不再有交，则返回环境光即可
			return La;

		// 针对粗糙材质，计算漫反射
		if (hit.material->type == ROUGH)
		{
			// 初始化返回光线（或者阴影）
			vec3 outRadiance = hit.material->ka * La;		
			for (auto& light : lights)
			{
				Ray shadowRay(hit.position + hit.normal * epsilon, light->direction);
				float cosTheta = dot(hit.normal, light->direction);
				// 如果cos小于0（钝角），说明光照到的是物体背面，用户看不到
				if (cosTheta > 0)	
				{
					// 如果与其他物体有交，则处于阴影中；反之按Phong模型计算
					if (!shadowIntersect(shadowRay))	
					{
						outRadiance = outRadiance + light->Le * hit.material->kd * cosTheta;
						vec3 halfway = normalize(-ray.dir + light->direction);
						float cosDelta = dot(hit.normal, halfway);
						if (cosDelta > 0)
							outRadiance = outRadiance + light->Le * hit.material->ks * powf(cosDelta, hit.material->shininess);
					}
				}
			}
			return outRadiance;
		}

		// 镜面反射（继续追踪）
		float cosa = -dot(ray.dir, hit.normal);		
		vec3 one(1, 1, 1);
		vec3 F = hit.material->F0 + (one - hit.material->F0) * pow(1 - cosa, 5);
		vec3 reflectedDir = ray.dir - hit.normal * dot(hit.normal, ray.dir) * 2.0f;		// 反射光线R = v + 2Ncosa
		
		// 递归调用trace()
		vec3 outRadiance = trace(Ray(hit.position + hit.normal * epsilon, reflectedDir), depth + 1) * F;

		// 对于透明物体，计算折射（继续追踪）
		if (hit.material->type == REFRACTIVE)	 
		{
			float disc = 1 - (1 - cosa * cosa) / hit.material->ior / hit.material->ior;
			if (disc >= 0)
			{
				vec3 refractedDir = ray.dir / hit.material->ior + hit.normal * (cosa / hit.material->ior - sqrt(disc));
				
				// 递归调用trace()
				outRadiance = outRadiance + trace(Ray(hit.position - hit.normal * epsilon, refractedDir), depth + 1) * (one - F);
			}
		}

		return outRadiance;
	}

	// 动画，相机绕场景旋转
	void animate(float dt)
	{
		viewPoint.animate(dt);
	}

	// 视点变化
	void zoomInOut(float dz)
	{
		viewPoint.zoomInOut(dz);
	}
};

// 场景对象
Scene scene;

//---------------------------
// 用来初始化顶点着色器与片段着色器的类，其构造函数中生成了VertexArray与Buffer缓冲。
class FullScreenTexturedQuad {
	// texture id
	unsigned int textureId = 0;	

public:
	FullScreenTexturedQuad(uint32_t windowWidth, uint32_t windowHeight)
	{
		// 启用二维纹理
		glEnable(GL_TEXTURE_2D);
		// 生成纹理标识符
		glGenTextures(1, &textureId);		
		// 绑定纹理
		glBindTexture(GL_TEXTURE_2D, textureId);	
		// 设置纹理参数
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// 加载纹理（光线追踪计算结果image作为纹理）
	void LoadTexture(vector<vec4>& image)
	{
		// 绑定纹理
		glBindTexture(GL_TEXTURE_2D, textureId);
		// 加载纹理（光线追踪计算颜色image）到纹理内存（to GPU）
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, &image[0]);	// To GPU
	}
};

FullScreenTexturedQuad* fullScreenTexturedQuad;

//---------------------------
// 整个函数的初始化，设定视窗、初始化场景、初始化着色器，并创建gpu进程
void onInitialization()
{
	// 设置视口变换
	glViewport(0, 0, windowWidth, windowHeight);
	
	// 建立场景
	scene.build();

	// 创建对象，初始化纹理
	fullScreenTexturedQuad = new FullScreenTexturedQuad(windowWidth, windowHeight);
}

long timebase=0;
long time_last_frame;
void showFrame()
{
    static long frame=0;
	// 计算帧率
    frame++;
    long time = glutGet(GLUT_ELAPSED_TIME);
    time_last_frame = time;

    if (time - timebase > 1000) {
        float fps = frame*1000.0/(time-timebase);
        timebase = time;
        frame = 0;

		cout << "FPS:" << fps << endl;
	}
}

//---------------------------------------------
// 显示函数。
//   调用render求取光线追踪的结果，存到image中，
//   然后用LoadTexture更新当前帧的纹理，最后绘制出来
void onDisplay()
{
	showFrame();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vector<vec4> image(windowWidth * windowHeight);
	
	// 场景绘制（通过光线追踪计算所有像素值，保存在image中）
	scene.render(image);

    //使用这种方法的话画面不能缩放
    //glDrawPixels(windowWidth, windowHeight, GL_RGBA, GL_FLOAT, &image[0]);
	
	// 把光线追踪计算的image作为场景纹理
	fullScreenTexturedQuad->LoadTexture(image);

	// 绘制纹理
	glBegin(GL_POLYGON);
		// 设置纹理坐标与顶点坐标
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0, -1.0, 0.0);
		
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0, 1.0, 0.0);
		
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0, 1.0, 0.0);
		
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0, -1.0, 0.0);
	glEnd();

	glutSwapBuffers();

}

void onReshape(int w, int h) {
	glViewport(0, 0, w, h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (w <= h)
		glOrtho(-1.0, 1.0, -1.0 * (GLfloat) h / (GLfloat) w,
				1.0 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
	else
		glOrtho(-1.0 * (GLfloat) w / (GLfloat) h,
				1.0 * (GLfloat) w / (GLfloat) h, -1.0, 1.0, -10.0, 10.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	timebase = glutGet(GLUT_ELAPSED_TIME);
}

void onIdle()
{
	// 视点旋转
    long time = glutGet(GLUT_ELAPSED_TIME);
	scene.animate(0.003f * (time - time_last_frame));

	glutPostRedisplay();
}

void onKeyboard(unsigned char key, int pX, int pY) 
{
	if (key == '1')
		glutIdleFunc(onIdle);
	if (key == '2')
		glutIdleFunc(NULL);
    
    if (key == 's'){
        long time = glutGet(GLUT_ELAPSED_TIME);
        scene.zoomInOut(0.05f * (time - time_last_frame));
    }
    
    if (key == 'w'){
        long time = glutGet(GLUT_ELAPSED_TIME);
        scene.zoomInOut(-0.05f * (time - time_last_frame));
    }
}

//-----------------------------------------
// Entry point of the application
int main(int argc, char* argv[]) {
	// Initialize GLUT, Glew and OpenGL 
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
    glutCreateWindow("Ray Tracing");

	glewExperimental = true;
	glewInit();
	// Initialize this program and create shaders
	onInitialization();

	// Register event handlers
	glutReshapeFunc(onReshape);
	glutDisplayFunc(onDisplay);   
	glutIdleFunc(onIdle);
	// 键盘
	glutKeyboardFunc(onKeyboard);

	glutMainLoop();
	
	return 1;
}