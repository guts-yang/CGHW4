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
    Vector3f ambient_light;

    Scene() : background_color(0.1f, 0.1f, 0.1f), ambient_light(0.1f, 0.1f, 0.1f)
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


/** �ݹ�ʵ��һ�����߸��٣����ظ����߶�Ӧ�����ص���ɫֵ
* @param [in] : ray_data, ����
* @param [in] : scene_data, ��������
* @param [in] : depth, �ݹ���ȣ�ÿ�ݹ�һ�Σ����ֵ��ȥ1
* @return : ���߶�Ӧ�����ص���ɫ
*/
Vector3f trace_ray(const ray& ray_data, const Scene& scene_data, int depth);


/** ������������ÿ�����ؼ�����ɫֵ
* @param [in] : scene_data����������
* @param [out] : image�������Ⱦ�����ͼƬ
* @param [in] : width, ��ȾͼƬ�Ŀ� (������
* @param [in] : height, ��ȾͼƬ�ĸ߶� (����)
* @param [in] : max_depth, ���ݹ����
*/
void render_scene(const Scene& scene_data, Mat& image, int width, int height, int max_depth);



#endif