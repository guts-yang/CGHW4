# 光线追踪渲染器项目

## 项目概述

这是一个基于C++实现的光线追踪渲染器，能够生成包含光照、阴影、反射和折射效果的3D场景图像。项目使用Eigen库进行向量和矩阵运算，使用OpenCV库进行图像显示和保存。

## 核心实现

### 1. 光线追踪算法

光线追踪是一种真实感渲染技术，通过模拟光线在场景中的传播、反射和折射来生成图像。本项目实现了以下核心功能：

- **射线-物体相交检测**：计算射线与球体、立方体和平面的相交情况
- **局部光照模型**：实现Phong光照模型，包含漫反射和镜面反射
- **阴影计算**：通过阴影射线检测光源遮挡
- **全局光照效果**：实现反射和折射的递归计算

### 2. 场景管理

场景由以下元素组成：
- **相机**：定义视角和投影方式
- **光源**：点光源，具有位置、颜色和强度属性
- **几何体**：包括球体、立方体和平面
- **材质**：定义物体表面的视觉属性，如漫反射颜色、镜面反射颜色、反射率、透射率等

## 重点分析：trace_ray函数实现

`trace_ray`函数是光线追踪算法的核心，实现了完整的光照计算和递归光线追踪逻辑。以下是对该函数的详细分析：

### 基本流程

1. **递归深度控制**：限制递归深度，避免无限递归
2. **交点检测**：找到光线与场景中物体的最近交点
3. **背景处理**：如果没有交点，返回背景颜色
4. **光照计算**：实现了完整的光照公式 `I = Ic + Ks*Ir + Kt*It`
   - `Ic`：局部光照模型计算的光强
   - `Ir`：环境的镜面反射光
   - `It`：环境的规则透射光
   - `Ks`：表面的镜面反射率
   - `Kt`：表面的透射率

### 关键技术实现

#### 1. 局部光照模型 (Ic)

```cpp
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
        // 漫反射计算
        float diffuse_factor = max(0.0f, intersection.normal.dot(light_dir));
        Ic += material.diffuse.cwiseProduct(light.color) * light.intensity * diffuse_factor;
        
        // 镜面反射计算
        if (material.shininess > 0 && material.specular.norm() > 0)
        {
            Vector3f view_dir = (-ray_data.direction).normalized();
            Vector3f reflect_dir = light_dir - 2.0f * light_dir.dot(intersection.normal) * intersection.normal;
            float specular_factor = pow(max(0.0f, view_dir.dot(reflect_dir)), material.shininess);
            Ic += material.specular.cwiseProduct(light.color) * light.intensity * specular_factor;
        }
    }
}
```

该部分实现了经典的Phong光照模型，包括：
- **自发光**：物体本身发出的光
- **阴影检测**：通过阴影射线确定点是否在光源的阴影中
- **漫反射**：基于Lambert余弦定律计算
- **镜面反射**：基于Phong模型计算，使用半程向量优化

#### 2. 镜面反射 (Ir)

```cpp
if (Ks > 0)
{
    // 计算反射方向
    Vector3f reflect_dir = ray_data.direction - 2.0f * ray_data.direction.dot(intersection.normal) * intersection.normal;
    // 生成反射射线
    ray reflect_ray(intersection.point + intersection.normal * 0.001f, reflect_dir);
    // 递归计算反射光
    Ir = trace_ray(reflect_ray, scene_data, depth);
}
```

该部分实现了反射光线的生成和递归追踪：
- 使用向量反射公式计算反射方向
- 添加一个小偏移量(`0.001f * intersection.normal`)避免自相交
- 递归调用`trace_ray`计算反射光线的贡献

#### 3. 规则透射 (It)

```cpp
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
```

该部分实现了基于斯涅尔定律的折射计算：
- 计算折射率和入射角余弦
- 检查全内反射条件
- 使用斯涅尔定律计算折射方向
- 递归调用`trace_ray`计算透射光线的贡献

#### 4. 颜色综合与归一化

```cpp
// 应用光照计算公式：I = Ic + Ks * Ir + Kt * It
Vector3f final_color = Ic + Ks * Ir + Kt * It;

return final_color.cwiseMin(Vector3f(1, 1, 1)).cwiseMax(Vector3f(0, 0, 0));
```

最终根据公式综合所有光照分量，并对颜色值进行归一化处理，确保RGB分量在[0,1]范围内。

## 技术栈

- **C++**：核心编程语言
- **Eigen**：线性代数库，用于向量和矩阵运算
- **OpenCV**：图像处理库，用于图像显示和保存
- **Visual Studio**：项目开发环境

## 使用方法

1. 确保已安装Visual Studio和所需依赖库
2. 打开`HW.sln`解决方案文件
3. 编译并运行项目
4. 渲染结果将显示并保存为`ray_tracing_result.jpg`文件

## 项目结构

```
├── HW4/              # 主项目目录
│   ├── main.cpp      # 程序入口
│   ├── RayTracer.cpp # 光线追踪核心实现
│   ├── RayTracer.h   # 头文件定义
├── bin/              # 编译输出目录
│   ├── HW4.exe       # 可执行文件
│   ├── opencv_*.dll  # OpenCV依赖库
│   └── ray_tracing_result.jpg # 渲染结果
├── include/          # 头文件目录
│   ├── Eigen/        # Eigen库
│   └── opencv2/      # OpenCV头文件
└── lib/              # 库文件目录
    └── opencv_*.lib  # OpenCV库文件
```

## 优化方向

1. **性能优化**：
   - 实现空间加速结构（如BVH树）
   - 多线程并行渲染
   - 光线包追踪

2. **功能扩展**：
   - 添加纹理映射
   - 支持更多几何形状
   - 实现景深效果
   - 添加全局光照算法（如光子映射）

## 总结

本项目实现了一个功能完整的光线追踪渲染器，通过递归追踪光线与场景的交互，生成包含光照、阴影、反射和折射效果的真实感图像。核心算法基于经典的光线追踪理论，同时考虑了各种实际情况（如阴影、全内反射等），能够生成高质量的渲染结果。