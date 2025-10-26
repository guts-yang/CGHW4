#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <opencv2/opencv.hpp>
#include <Eigen/Dense>
#include <vector>
#include <memory>
#include <limits>
#include <cmath>

#define M_PI 3.14159265358979323846

using namespace cv;
using namespace Eigen;
using namespace std;

struct Material
{
    Vector3f diffuse;
    Vector3f specular;
    Vector3f emission;
    float shininess;
    float reflectivity;
    float transparency;
    float refractive_index;

    Material() : diffuse(0, 0, 0), specular(0, 0, 0), emission(0, 0, 0), shininess(0), reflectivity(0), transparency(0), refractive_index(1)
    {
    }
};

struct ray
{
    Vector3f origin;
    Vector3f direction;

    ray(const Vector3f& o, const Vector3f& d) : origin(o), direction(d.normalized())
    {
    }
};

struct Intersection
{
    bool hit;
    float distance;
    Vector3f point;
    Vector3f normal;
    Material material_data;

    Intersection() : hit(false), distance(numeric_limits<float>::max())
    {
    }
};

class Object
{
public:
    Material material_data;

    virtual Intersection intersect(const ray& ray_data) const = 0;

    virtual ~Object()
    {
    }
};

class Sphere : public Object
{
public:
    Vector3f center;
    float radius;

    Sphere(const Vector3f& c, float r) : center(c), radius(r)
    {
    }

    Intersection intersect(const ray& ray_data) const override;
};

class Cube : public Object
{
public:
    Vector3f min_point;
    Vector3f max_point;

    Cube(const Vector3f& min, const Vector3f& max) : min_point(min), max_point(max)
    {
    }

    Intersection intersect(const ray& ray_data) const override;
};

class plane : public Object
{
public:
    Vector3f point;
    Vector3f normal;

    plane(const Vector3f& p, const Vector3f& n) : point(p), normal(n.normalized())
    {
    }

    Intersection intersect(const ray& ray_data) const override;
};

class Light
{
public:
    Vector3f position;
    Vector3f color;
    float intensity;

    Light(const Vector3f& pos, const Vector3f& col, float intens) : position(pos), color(col), intensity(intens)
    {
    }
};

class Scene
{
public:
    vector<shared_ptr<Object>> objects;
    vector<Light> lights;
    Vector3f background_color;

    Scene() : background_color(0.1f, 0.1f, 0.1f)
    {
    }

    void add_object(shared_ptr<Object> object_ptr)
    {
        objects.push_back(object_ptr);
    }

    void add_light(const Light& light_data)
    {
        lights.push_back(light_data);
    }

    Intersection find_closest_intersection(const ray& ray_data) const;
};


/** 递归实现一条光线跟踪，返回该射线对应的像素的颜色值
* @param [in] : ray_data, 射线
* @param [in] : scene_data, 场景数据
* @param [in] : depth, 递归深度，每递归一次，深度值减去1
* @return : 射线对应的像素的颜色
*/
Vector3f trace_ray(const ray& ray_data, const Scene& scene_data, int depth);


/** 对整个场景的每个像素计算颜色值
* @param [in] : scene_data，场景数据
* @param [out] : image，最后渲染保存的图片
* @param [in] : width, 渲染图片的宽 (列数）
* @param [in] : height, 渲染图片的高度 (行数)
* @param [in] : max_depth, 最大递归深度
*/
void render_scene(const Scene& scene_data, Mat& image, int width, int height, int max_depth);



#endif