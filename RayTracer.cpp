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

        // 计算法向量
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


/** 递归实现一条光线跟踪，返回该射线对应的像素的颜色值
* @param [in] : ray_data, 射线
* @param [in] : scene_data, 场景数据
* @param [in] : depth, 递归深度，每递归一次，深度值减去1
* @return : 射线对应的像素的颜色
*/

Vector3f trace_ray(const ray& ray_data, const Scene& scene_data, int depth)
{
    // 每递归调用一次，就减少1
    --depth;
    //--- 深度小于0 ， 则返回 ---//
    if (depth <= 0)
    {
        return Vector3f(0, 0, 0);
    }

    // 找到最近的交点
    Intersection intersection = scene_data.find_closest_intersection(ray_data);
    
    // 如果没有找到交点，则返回背景颜色
    if (!intersection.hit)
    {
        return scene_data.background_color;
    }

    Vector3f Ic(0, 0, 0);  // 局部光照模型计算的光强
    Vector3f Ir(0, 0, 0);  // 环境的镜面反射光
    Vector3f It(0, 0, 0);  // 环境的规则透射光
    
    const Material& material = intersection.material_data;
    float Ks = material.reflectivity;   // 表面的镜面反射率
    float Kt = material.transparency;   // 表面的透射率
    
    // 1. 计算局部光照模型Ic
    // 添加自发光颜色
    Ic += material.emission;
    
    // 计算每个光源对交点的贡献
    for (const Light& light : scene_data.lights)
    {
        // 计算光线方向和距离
        Vector3f light_dir = (light.position - intersection.point).normalized();
        float light_distance = (light.position - intersection.point).norm();
        
        // 生成阴影射线，检查是否在阴影中
        ray shadow_ray(intersection.point + intersection.normal * 0.001f, light_dir);
        Intersection shadow_intersection = scene_data.find_closest_intersection(shadow_ray);
        
        // 如果没有被遮挡，则计算光照贡献
        if (!shadow_intersection.hit || shadow_intersection.distance > light_distance)
        {
            // 计算漫反射
            float diffuse_factor = max(0.0f, intersection.normal.dot(light_dir));
            Ic += material.diffuse.cwiseProduct(light.color) * light.intensity * diffuse_factor;
            
            // 计算镜面反射
            if (material.shininess > 0 && material.specular.norm() > 0)
            {
                Vector3f view_dir = (-ray_data.direction).normalized();
                Vector3f reflect_dir = light_dir - 2.0f * light_dir.dot(intersection.normal) * intersection.normal;
                float specular_factor = pow(max(0.0f, view_dir.dot(reflect_dir)), material.shininess);
                Ic += material.specular.cwiseProduct(light.color) * light.intensity * specular_factor;
            }
        }
    }
    
    // 2. 计算环境的镜面反射光Ir
    if (Ks > 0)
    {
        // 计算反射方向
        Vector3f reflect_dir = ray_data.direction - 2.0f * ray_data.direction.dot(intersection.normal) * intersection.normal;
        // 生成反射射线
        ray reflect_ray(intersection.point + intersection.normal * 0.001f, reflect_dir);
        // 递归计算反射光
        Ir = trace_ray(reflect_ray, scene_data, depth);
    }
    
    // 3. 计算环境的规则透射光It
    if (Kt > 0)
    {
        // 计算折射方向（使用斯涅尔定律）
        float eta = 1.0f / material.refractive_index; // 假设从空气进入物体
        float cosi = -ray_data.direction.dot(intersection.normal);
        float cost_sq = 1.0f - eta * eta * (1.0f - cosi * cosi);
        
        // 检查全内反射
        if (cost_sq > 0)
        {
            Vector3f refract_dir = eta * ray_data.direction + (eta * cosi - sqrt(cost_sq)) * intersection.normal;
            // 生成折射射线
            ray refract_ray(intersection.point - intersection.normal * 0.001f, refract_dir);
            // 递归计算透射光
            It = trace_ray(refract_ray, scene_data, depth);
        }
    }
    
    // 应用光照计算公式：I = Ic + Ks * Ir + Kt * It
    Vector3f final_color = Ic + Ks * Ir + Kt * It;
    
    return final_color.cwiseMin(Vector3f(1, 1, 1)).cwiseMax(Vector3f(0, 0, 0)); // 归一化处理颜色，保证是在0-1之间
}



/** 对整个场景的每个像素计算颜色值
* @param [in] : scene_data，场景数据
* @param [out] : image，最后渲染保存的图片
* @param [in] : width, 渲染图片的宽 (列数）
* @param [in] : height, 渲染图片的高度 (行数)
* @param [in] : max_depth, 最大递归深度
*/
void render_scene(const Scene& scene_data, Mat& image, int width, int height, int max_depth)
{
    // 调整相机位置，确保能看到所有物体
    Vector3f camera_pos(0, 6, -10);  // 相机在房间前方稍高的位置
    Vector3f look_at(0, 0, 10);      // 看向房间深处
    Vector3f up_dir(0, 1, 0);

    Vector3f forward = (look_at - camera_pos).normalized();
    Vector3f right = forward.cross(up_dir).normalized();
    Vector3f actual_up = right.cross(forward);

    float fov = 60.0f * M_PI / 180.0f;  // 增大视野
    float aspect_ratio = float(width) / float(height);
    float scale = tan(fov * 0.5f);

    //--- 对每个像素进行计算 ---//
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

        // 显示进度
        if (y % 50 == 0)
        {
            cout << "渲染进度: " << (y * 100 / height) << "%" << endl;
        }
    }
}


