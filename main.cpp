#include "RayTracer.h"
#include <iostream>


void setup_scene(Scene& scene_data);

void setup_scene(Scene& scene_data)
{

    //--- �������䳡�� ---//
    auto floor_plane = make_shared<plane>(Vector3f(0, -5, 0), Vector3f(0, 1, 0));
    floor_plane->material_data.diffuse = Vector3f(0.3f, 0.5f, 0.2f);
    floor_plane->material_data.shininess = 16.0f;
    scene_data.add_object(floor_plane);

    auto ceiling_plane = make_shared<plane>(Vector3f(0, 15, 0), Vector3f(0, -1, 0));
    ceiling_plane->material_data.diffuse = Vector3f(0.5f, 0.5f, 0.5f);
    scene_data.add_object(ceiling_plane);

    auto left_wall = make_shared<plane>(Vector3f(-15, 0, 0), Vector3f(1, 0, 0));
    left_wall->material_data.diffuse = Vector3f(0.7f, 0.3f, 0.3f);
    scene_data.add_object(left_wall);

    auto right_wall = make_shared<plane>(Vector3f(15, 0, 0), Vector3f(-1, 0, 0));
    right_wall->material_data.diffuse = Vector3f(0.3f, 0.5f, 0.7f);
    scene_data.add_object(right_wall);

    auto front_wall = make_shared<plane>(Vector3f(0, 0, 20), Vector3f(0, 0, -1));
    front_wall->material_data.diffuse = Vector3f(0.6f, 0.1f, 0.3f);
    scene_data.add_object(front_wall);

    auto back_wall = make_shared<plane>(Vector3f(0, 0, -20), Vector3f(0, 0, 1));
    back_wall->material_data.diffuse = Vector3f(0.6f, 0.6f, 0.1f);
    scene_data.add_object(back_wall);

    //--- ��������ģ�� ---//

    //--- �������м䣩
    auto glass_sphere = make_shared<Sphere>(Vector3f(0, 0, 5), 2.0f);
    glass_sphere->material_data.diffuse = Vector3f(0.1f, 0.1f, 0.1f);
    glass_sphere->material_data.specular = Vector3f(1.0f, 1.0f, 1.0f);
    glass_sphere->material_data.transparency = 0.8f;
    glass_sphere->material_data.reflectivity = 0.2f;
    glass_sphere->material_data.refractive_index = 1.5f;
    glass_sphere->material_data.shininess = 128.0f;
    scene_data.add_object(glass_sphere);

    //--- ����һ����
    auto green_sphere = make_shared<Sphere>(Vector3f(-6, 0, 8), 2.0f);
    green_sphere->material_data.diffuse = Vector3f(0.2f, 0.8f, 0.2f);
    green_sphere->material_data.specular = Vector3f(0.5f, 0.5f, 0.5f);
    green_sphere->material_data.shininess = 64.0f;
    green_sphere->material_data.reflectivity = 0.3f;
    scene_data.add_object(green_sphere);

    //--- ������ 
    auto blue_cube = make_shared<Cube>(Vector3f(4, -2, 6), Vector3f(8, 2, 10));
    blue_cube->material_data.diffuse = Vector3f(0.2f, 0.6f, 0.8f);
    blue_cube->material_data.specular = Vector3f(0.4f, 0.4f, 0.4f);
    blue_cube->material_data.shininess = 32.0f;
    blue_cube->material_data.reflectivity = 0.2f;
    scene_data.add_object(blue_cube);

    //--- ���ù�Դ ---//
    scene_data.add_light(Light(Vector3f(0, 14, 5), Vector3f(1.0f, 1.0f, 1.0f), 1.0f));  // �컨������
    scene_data.add_light(Light(Vector3f(-10, 10, 0), Vector3f(0.9f, 0.9f, 0.7f), 0.7f)); // ���
    scene_data.add_light(Light(Vector3f(10, 10, 0), Vector3f(0.2f, 0.9f, 0.9f), 0.7f));  // �Ҳ�
}

int main()
{
    cout << "��ʼ��ʼ������..." << endl;
    Scene main_scene;
    setup_scene(main_scene);
    cout << "������ʼ�����" << endl;

    int image_width   = 800;  // ��ȾͼƬ�Ŀ�
    int image_height  = 600;  // ��ȾͼƬ�ĸ�
    int max_ray_depth = 5;    // �������ݹ����

    cout << "��ʼ��Ⱦͼ��..." << endl;
    Mat output_image(image_height, image_width, CV_32FC3, Scalar(0, 0, 0));

    //--- ��Ⱦ���� ---//
    render_scene(main_scene, output_image, image_width, image_height, max_ray_depth);



    Mat display_image;
    output_image.convertTo(display_image, CV_8UC3, 255.0);

    cout << "��Ⱦ��ɣ���ʾͼ��..." << endl;
    imshow("ray Tracing Result", display_image);
    waitKey(0);

    imwrite("ray_tracing_result.jpg", display_image);
    cout << "ͼ���ѱ���Ϊ ray_tracing_result.jpg" << endl;

    return 0;
}