#include <opencv2/opencv.hpp>
#include <iostream>
#include "RayTracer.h"

using namespace cv;
using namespace std;
using namespace Eigen;

int main()
{
    cout << "开始光线追踪渲染..." << endl;
    
    int image_width = 800;
    int image_height = 600;
    
    // 创建场景
    Scene scene;
    scene.background_color = Vector3f(0.1f, 0.1f, 0.1f);
    scene.ambient_light = Vector3f(0.2f, 0.2f, 0.2f);
    
    // 添加光源 - 使用明亮的光源
    scene.add_light(Light(Vector3f(5, 10, -5), Vector3f(1, 1, 1), 50));
    scene.add_light(Light(Vector3f(-5, 10, -5), Vector3f(1, 1, 1), 50));
    
    // 创建地板平面 - 绿色地面
    auto floor = make_shared<plane>(Vector3f(0, -2, 0), Vector3f(0, 1, 0));
    floor->material_data.diffuse = Vector3f(0.2f, 0.8f, 0.2f);
    floor->material_data.specular = Vector3f(0.1f, 0.1f, 0.1f);
    floor->material_data.shininess = 10.0f;
    scene.add_object(floor);
    
    // 创建后面的墙壁 - 粉色背景墙
    auto back_wall = make_shared<plane>(Vector3f(0, 0, 15), Vector3f(0, 0, -1));
    back_wall->material_data.diffuse = Vector3f(0.9f, 0.2f, 0.5f);
    back_wall->material_data.specular = Vector3f(0.1f, 0.1f, 0.1f);
    back_wall->material_data.shininess = 10.0f;
    scene.add_object(back_wall);
    
    // 创建左边的墙壁 - 蓝色
    auto left_wall = make_shared<plane>(Vector3f(-8, 0, 0), Vector3f(1, 0, 0));
    left_wall->material_data.diffuse = Vector3f(0.2f, 0.2f, 0.9f);
    left_wall->material_data.specular = Vector3f(0.1f, 0.1f, 0.1f);
    left_wall->material_data.shininess = 10.0f;
    scene.add_object(left_wall);
    
    // 创建红色反光球体
    auto red_sphere = make_shared<Sphere>(Vector3f(-3, 0, 5), 1.5f);
    red_sphere->material_data.diffuse = Vector3f(0.8f, 0.1f, 0.1f);
    red_sphere->material_data.specular = Vector3f(0.9f, 0.9f, 0.9f);
    red_sphere->material_data.shininess = 200.0f;
    red_sphere->material_data.reflectivity = 0.3f;
    scene.add_object(red_sphere);
    
    // 创建绿色玻璃球体
    auto green_sphere = make_shared<Sphere>(Vector3f(3, 0, 5), 1.5f);
    green_sphere->material_data.diffuse = Vector3f(0.1f, 0.8f, 0.1f);
    green_sphere->material_data.specular = Vector3f(0.9f, 0.9f, 0.9f);
    green_sphere->material_data.shininess = 200.0f;
    green_sphere->material_data.transparency = 0.8f;
    green_sphere->material_data.refractive_index = 1.5f;
    scene.add_object(green_sphere);
    
    // 创建蓝色立方体
    auto blue_cube = make_shared<Cube>(Vector3f(-2, -2, 2), Vector3f(0, 0, 4));
    blue_cube->material_data.diffuse = Vector3f(0.1f, 0.3f, 0.9f);
    blue_cube->material_data.specular = Vector3f(0.9f, 0.9f, 0.9f);
    blue_cube->material_data.shininess = 100.0f;
    blue_cube->material_data.reflectivity = 0.5f;
    scene.add_object(blue_cube);
    
    // 创建图像
    Mat image(image_height, image_width, CV_32FC3, Scalar(0, 0, 0));
    
    // 渲染场景
    render_scene(scene, image, image_width, image_height, 5);
    
    // 将浮点型图像转换为8位图像以便保存
    Mat output_image;
    image.convertTo(output_image, CV_8UC3, 255.0);
    
    // 保存图像
    bool save_success = imwrite("ray_tracing_result.jpg", output_image);
    
    if (save_success)
    {
        cout << "渲染完成！图像已保存为 ray_tracing_result.jpg" << endl;
    }
    else
    {
        cout << "保存图像失败！请检查OpenCV配置和文件权限。" << endl;
    }
    
    return 0;
}