#include "RayTracer.h"

Intersection Sphere::intersect(const ray& ray_data) const
{
    Intersection result;
    Vector3f oc = ray_data.origin - center;
    float a = ray_data.direction.dot(ray_data.direction);
    float b = 2.0f * oc.dot(ray_data.direction);
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant >= 0)
    {
        float sqrt_discriminant = sqrt(discriminant);
        float t1 = (-b - sqrt_discriminant) / (2.0f * a);
        float t2 = (-b + sqrt_discriminant) / (2.0f * a);

        float t = min(t1, t2);
        if (t < 0.001f)
        {
            t = max(t1, t2);
        }

        if (t > 0.001f && t < result.distance)
        {
            result.hit = true;
            result.distance = t;
            result.point = ray_data.origin + t * ray_data.direction;
            result.normal = (result.point - center).normalized();
            result.material_data = material_data;
        }
    }

    return result;
}

Intersection Cube::intersect(const ray& ray_data) const
{
    Intersection result;

    Vector3f t1 = (min_point - ray_data.origin).array() / ray_data.direction.array();
    Vector3f t2 = (max_point - ray_data.origin).array() / ray_data.direction.array();

    Vector3f t_min = t1.cwiseMin(t2);
    Vector3f t_max = t1.cwiseMax(t2);

    float t_near = max(max(t_min.x(), t_min.y()), t_min.z());
    float t_far = min(min(t_max.x(), t_max.y()), t_max.z());

    if (t_near > t_far || t_far < 0.001f)
    {
        return result;
    }

    float t = (t_near > 0.001f) ? t_near : t_far;

    if (t > 0.001f && t < result.distance)
    {
        result.hit = true;
        result.distance = t;
        result.point = ray_data.origin + t * ray_data.direction;

        // ????????
        const float epsilon = 0.0001f;
        if (fabs(result.point.x() - min_point.x()) < epsilon)
        {
            result.normal = Vector3f(-1, 0, 0);
        }
        else if (fabs(result.point.x() - max_point.x()) < epsilon)
        {
            result.normal = Vector3f(1, 0, 0);
        }
        else if (fabs(result.point.y() - min_point.y()) < epsilon)
        {
            result.normal = Vector3f(0, -1, 0);
        }
        else if (fabs(result.point.y() - max_point.y()) < epsilon)
        {
            result.normal = Vector3f(0, 1, 0);
        }
        else if (fabs(result.point.z() - min_point.z()) < epsilon)
        {
            result.normal = Vector3f(0, 0, -1);
        }
        else
        {
            result.normal = Vector3f(0, 0, 1);
        }

        result.material_data = material_data;
    }

    return result;
}

Intersection plane::intersect(const ray& ray_data) const
{
    Intersection result;
    float denom = normal.dot(ray_data.direction);

    if (fabs(denom) > 0.0001f)
    {
        float t = (point - ray_data.origin).dot(normal) / denom;

        if (t > 0.001f && t < result.distance)
        {
            result.hit = true;
            result.distance = t;
            result.point = ray_data.origin + t * ray_data.direction;
            result.normal = normal;
            result.material_data = material_data;
        }
    }

    return result;
}

Intersection Scene::find_closest_intersection(const ray& ray_data) const
{
    Intersection closest;

    for (const auto& obj : objects)
    {
        Intersection current_intersection = obj->intersect(ray_data);

        if (current_intersection.hit && current_intersection.distance < closest.distance)
        {
            closest = current_intersection;
        }
    }

    return closest;
}


/** ???????????????????????????????????????
* @param [in] : ray_data, ????
* @param [in] : scene_data, ????????
* @param [in] : depth, ?????????????Σ????????1
* @return : ????????????????
*/
Vector3f trace_ray(const ray& ray_data, const Scene& scene_data, int depth)
{
    //--- ???С??0 ?? ??? ---//
    if (depth <= 0)
    {
        return Vector3f(0, 0, 0);
    }
    
    // ?????????Σ??????1
    -- depth;

    Vector3f final_color(0, 0, 0);

    //--- ??????????????? ---
    Intersection intersection = scene_data.find_closest_intersection(ray_data);
    
    // ???????交点??????景色
    if (!intersection.hit)
    {
        return scene_data.background_color;
    }
    
    // ????局部光照（阴影、漫反射、镜面反射）
    Vector3f local_color(0, 0, 0);
    
    // 添加环境光
    local_color += scene_data.ambient_light * intersection.material_data.diffuse * 0.3f;
    
    // ????自发光
    local_color += intersection.material_data.emission;
    
    // ???每个光源对交点的贡献
    for (const auto& light : scene_data.lights)
    {
        // ???光线从交点指向光源的向量
        Vector3f light_dir = (light.position - intersection.point).normalized();
        float light_distance = (light.position - intersection.point).norm();
        
        // 检查阴影
        bool in_shadow = false;
        ray shadow_ray(intersection.point + intersection.normal * 0.001f, light_dir);
        Intersection shadow_intersection = scene_data.find_closest_intersection(shadow_ray);
        
        if (shadow_intersection.hit && shadow_intersection.distance < light_distance)
        {
            in_shadow = true;
        }
        
        if (!in_shadow)
        {
            // ???漫反射
            float diff_coeff = max(0.0f, intersection.normal.dot(light_dir));
            // 提高漫反射的贡献，使物体更明亮
            local_color += light.color * light.intensity * intersection.material_data.diffuse * diff_coeff * 1.5f;
            
            // ???镜面反射 - 使用Blinn-Phong模型，更稳定
            Vector3f view_dir = -ray_data.direction.normalized();
            Vector3f halfway = (light_dir + view_dir).normalized();
            float spec_coeff = pow(max(0.0f, intersection.normal.dot(halfway)), intersection.material_data.shininess);
            local_color += light.color * light.intensity * intersection.material_data.specular * spec_coeff;
        }
    }
    
    // 处理反射
    if (intersection.material_data.reflectivity > 0.0f)
    {
        Vector3f reflect_dir = ray_data.direction - 2 * ray_data.direction.dot(intersection.normal) * intersection.normal;
        ray reflect_ray(intersection.point + intersection.normal * 0.001f, reflect_dir);
        Vector3f reflect_color = trace_ray(reflect_ray, scene_data, depth);
        local_color += reflect_color * intersection.material_data.reflectivity;
    }
    
    // 处理折射
    if (intersection.material_data.transparency > 0.0f)
    {
        float eta = 1.0f / intersection.material_data.refractive_index;
        float cosi = -ray_data.direction.dot(intersection.normal);
        Vector3f n = intersection.normal;
        
        // ???光线是从物体内部射出的，调整折射系数和法线方向
        if (cosi < 0)
        {
            cosi = -cosi;
            eta = 1.0f / eta;
            n = -n; // 反转法线方向
        }
        
        float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
        
        if (k > 0)
        {
            Vector3f refract_dir = eta * ray_data.direction + (eta * cosi - sqrt(k)) * n;
            ray refract_ray(intersection.point + n * 0.001f, refract_dir);
            Vector3f refract_color = trace_ray(refract_ray, scene_data, depth);
            local_color += refract_color * intersection.material_data.transparency;
        }
    }
    
    final_color = local_color;

    return final_color.cwiseMin(Vector3f(1, 1, 1)).cwiseMax(Vector3f(0, 0, 0)); // ?????????????????????0-1???
}


/** ??????????????????????????
* @param [in] : scene_data??????????
* @param [out] : image???????????????
* @param [in] : width, ???????? (??????
* @param [in] : height, ????????? (????)
* @param [in] : max_depth, ????????
*/
void render_scene(const Scene& scene_data, Mat& image, int width, int height, int max_depth)
{
    // ???????λ???????????????????
    Vector3f camera_pos(0, 6, -10);  // ???????????????λ??
    Vector3f look_at(0, 0, 10);      // ???????
    Vector3f up_dir(0, 1, 0);

    Vector3f forward = (look_at - camera_pos).normalized();
    Vector3f right = forward.cross(up_dir).normalized();
    Vector3f actual_up = right.cross(forward);

    float fov = 60.0f * M_PI / 180.0f;  // ???????
    float aspect_ratio = float(width) / float(height);
    float scale = tan(fov * 0.5f);

    //--- ???????????м??? ---//
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
           
            float pixel_x = (2.0f * (x + 0.5f) / width - 1.0f) * aspect_ratio * scale;
            float pixel_y = (1.0f - 2.0f * (y + 0.5f) / height) * scale;

            Vector3f ray_dir = (forward + right * pixel_x + actual_up * pixel_y).normalized();
            ray current_ray(camera_pos, ray_dir);

            Vector3f pixel_color = trace_ray(current_ray, scene_data, max_depth);

            image.at<Vec3f>(y, x) = Vec3f(pixel_color[2], pixel_color[1], pixel_color[0]);

        }

        // ???????
        if (y % 50 == 0)
        {
            cout << "???????: " << (y * 100 / height) << "%" << endl;
        }
    }
}


