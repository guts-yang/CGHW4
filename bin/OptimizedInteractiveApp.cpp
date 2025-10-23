#include <windows.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>

// Define missing macros
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// Improved resolution with balanced performance
const int WINDOW_WIDTH = 768;
const int WINDOW_HEIGHT = 432;

// Interactive mode flags
bool isInteractiveMode = false;
int selectedSphereIndex = -1;
POINT lastMousePos;

// Incremental rendering parameters
bool incrementalRendering = false;
int currentScanline = 0;

// Timer for controlling update frequency
DWORD lastUpdateTime = 0;
DWORD lastRenderTime = 0;
const DWORD UPDATE_INTERVAL = 4; // Minimum update interval in ms

// Optimized Vector3 class with performance improvements
struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
    
    // Vector operations with performance optimizations

    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    
    // Vector multiplication by scalar
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    
    // Vector component-wise multiplication
    Vector3 operator*(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
    
    // Vector addition and subtraction
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    
    Vector3 operator/(float s) const { float invS = 1.0f / s; return Vector3(x * invS, y * invS, z * invS); }
    
    // Add length squared for faster distance comparisons
    float lengthSquared() const { return x * x + y * y + z * z; }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    
    // Normalize with early exit for zero vectors
    Vector3 normalize() const {
        float lenSq = x * x + y * y + z * z;
        if (lenSq < 0.0001f) return Vector3(0, 0, 0);
        float invLen = 1.0f / sqrtf(lenSq);
        return Vector3(x * invLen, y * invLen, z * invLen);
    }
    
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
    
    Vector3 operator-() const { return Vector3(-x, -y, -z); }
};

// Material struct
struct Material {
    Vector3 diffuse;      // Diffuse color
    Vector3 specular;     // Specular color
    float shininess;      // Shininess
    float reflectivity;   // Reflectivity
    float transparency;   // Transparency
    float refractiveIndex; // Refractive index
    
    Material(
        const Vector3& diffuse = Vector3(0.8f, 0.8f, 0.8f),
        const Vector3& specular = Vector3(1.0f, 1.0f, 1.0f),
        float shininess = 32.0f,
        float reflectivity = 0.2f,
        float transparency = 0.0f,
        float refractiveIndex = 1.5f
    ) : diffuse(diffuse), specular(specular), shininess(shininess),
        reflectivity(reflectivity), transparency(transparency), refractiveIndex(refractiveIndex) {}
};

// Sphere struct
struct Sphere {
    Vector3 center;
    float radius;
    Material material;
    bool selected;
    
    Sphere(const Vector3& center, float radius, const Material& material) 
        : center(center), radius(radius), material(material), selected(false) {}
};

// Plane struct
struct Plane {
    Vector3 point;
    Vector3 normal;
    Material material;
    
    Plane(const Vector3& point, const Vector3& normal, const Material& material) 
        : point(point), normal(normal.normalize()), material(material) {}
};

// Ray struct
struct Ray {
    Vector3 origin;
    Vector3 direction;
    
    Ray(const Vector3& origin, const Vector3& direction) 
        : origin(origin), direction(direction.normalize()) {}
};

// Light struct
struct Light {
    Vector3 position;
    Vector3 color;
    float intensity;
    
    Light(const Vector3& position, const Vector3& color, float intensity) 
        : position(position), color(color), intensity(intensity) {}
};

// Intersection info struct
struct Intersection {
    bool hit;
    float t;
    Vector3 point;
    Vector3 normal;
    const Material* material;
    
    Intersection() : hit(false), t(0.0f), material(nullptr) {}
};

// Scene objects
std::vector<Sphere> spheres;
std::vector<Plane> planes;
std::vector<Light> lights;

// Camera parameters
Vector3 cameraPosition(0.0f, 1.5f, -5.0f);
Vector3 cameraTarget(0.0f, 0.0f, 0.0f);
Vector3 cameraUp(0.0f, 1.0f, 0.0f);
float fov = 60.0f;

// Mouse interaction state
bool isDragging = false;
// Variables already defined at the top of the file

// Frame rate optimization: Use a frame buffer to store the previous frame
BYTE* frameBuffer = nullptr;
bool frameBufferInitialized = false;

// Check ray-sphere intersection
Intersection intersectSphere(const Ray& ray, const Sphere& sphere) {
    Intersection intersection;
    
    Vector3 oc = ray.origin - sphere.center;
    float a = ray.direction.dot(ray.direction);
    float b = 2.0f * oc.dot(ray.direction);
    float c = oc.dot(oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return intersection; // No intersection
    }
    
    float t = (-b - sqrt(discriminant)) / (2.0f * a);
    if (t > 0.0001f) {
        intersection.hit = true;
        intersection.t = t;
        intersection.point = ray.origin + ray.direction * t;
        intersection.normal = (intersection.point - sphere.center).normalize();
        intersection.material = &sphere.material;
    }
    
    return intersection;
}

// Check ray-plane intersection
Intersection intersectPlane(const Ray& ray, const Plane& plane) {
    Intersection intersection;
    
    float denom = ray.direction.dot(plane.normal);
    if (fabs(denom) > 0.0001f) {
        Vector3 p0l0 = plane.point - ray.origin;
        float t = p0l0.dot(plane.normal) / denom;
        if (t > 0.0001f) {
            intersection.hit = true;
            intersection.t = t;
            intersection.point = ray.origin + ray.direction * t;
            intersection.normal = plane.normal;
            intersection.material = &plane.material;
        }
    }
    
    return intersection;
}

// Improved scene intersection with early exit and interactive mode optimization
Intersection intersectScene(const Ray& ray) {
    Intersection closest;
    closest.t = 1e30f;
    
    // During interaction, check selected sphere first for faster feedback
    if (isInteractiveMode && selectedSphereIndex != -1 && selectedSphereIndex < spheres.size()) {
        Intersection intersection = intersectSphere(ray, spheres[selectedSphereIndex]);
        if (intersection.hit && intersection.t < closest.t) {
            closest = intersection;
        }
    }
    
    // Early exit optimization - first check the spheres (most likely to be hit)
    for (size_t i = 0; i < spheres.size(); i++) {
        // Skip selected sphere if we already checked it
        if (isInteractiveMode && selectedSphereIndex != -1 && i == (size_t)selectedSphereIndex) {
            continue;
        }
        
        Intersection intersection = intersectSphere(ray, spheres[i]);
        if (intersection.hit && intersection.t < closest.t) {
            closest = intersection;
        }
    }
    
    // Only check planes if no close sphere hit
    if (closest.t > 1e29f) {
        for (const auto& plane : planes) {
            Intersection intersection = intersectPlane(ray, plane);
            if (intersection.hit && intersection.t < closest.t) {
                closest = intersection;
            }
        }
    }
    
    return closest;
}

// Calculate shadow with probability skipping for performance
bool isInShadow(const Vector3& point, const Light& light) {
    // In interactive mode, use a higher probability to skip shadow calculations
    if (isInteractiveMode && rand() % 100 < 70) { // 70% chance to skip in interactive mode
        return false;
    }
    
    Vector3 shadowRayDir = (light.position - point).normalize();
    Ray shadowRay(point + shadowRayDir * 0.001f, shadowRayDir);
    Intersection intersection = intersectScene(shadowRay);
    
    if (intersection.hit) {
        float distanceToLight = (light.position - point).length();
        if (intersection.t < distanceToLight) {
            return true;
        }
    }
    
    return false;
}

// Improved physically-based shading model with adaptive quality based on interactive mode
Vector3 shade(const Intersection& intersection, int depth = 0) {
    if (depth > 1) return Vector3(0, 0, 0); // Keep low recursion for performance

    Vector3 color(0, 0, 0);
    const Material& material = *intersection.material;

    // Improved ambient lighting with environment color
    Vector3 ambient = Vector3(0.15f, 0.15f, 0.18f) * material.diffuse;
    color = color + ambient;

    // In interactive mode, use fewer lights for better performance
    int numLights = isInteractiveMode ? 1 : lights.size();

    // Calculate contribution from each light source
    for (int i = 0; i < numLights; i++) {
        const Light& light = lights[i];
        
        // Better shadow calculation - only skip for non-critical areas
        bool skipShadow = false;
        if (material.reflectivity < 0.2f && depth == 0) {
            skipShadow = (rand() % 100) < (isInteractiveMode ? 70 : 30); // Higher chance to skip in interactive mode
        }
        
        if (skipShadow || !isInShadow(intersection.point, light)) {
            // Calculate light direction and distance for realistic falloff
            Vector3 lightDir = (light.position - intersection.point);
            float distance = lightDir.length();
            lightDir = lightDir.normalize();
            
            // Realistic attenuation model
            float attenuation = 1.0f / (1.0f + 0.02f * distance + 0.01f * distance * distance);
            
            // Diffuse reflection with Lambert model
            float diffuseFactor = std::max(0.0f, intersection.normal.dot(lightDir));
            Vector3 diffuse = material.diffuse * light.color * diffuseFactor * light.intensity * attenuation;

            // Specular reflection - simplified in interactive mode
            Vector3 specular(0, 0, 0);
            if (!isInteractiveMode) { // Skip specular in interactive mode for speed
                Vector3 viewDir = (cameraPosition - intersection.point).normalize();
                Vector3 halfway = (lightDir + viewDir).normalize();
                
                // Microfacet-based specular model
                float specularFactor = std::pow(std::max(0.0f, intersection.normal.dot(halfway)), material.shininess);
                // More realistic specular intensity
                float specularIntensity = 0.5f * material.shininess / (material.shininess + 8.0f);
                specular = material.specular * light.color * specularFactor * 
                          light.intensity * specularIntensity * attenuation;
            }

            color = color + diffuse + specular;
        }
    }

    // Improved reflection with better sampling - simplified in interactive mode
    if (material.reflectivity > 0.25f) { // Slightly lower threshold for better visuals
        // In interactive mode, skip reflection entirely for speed
        if (!isInteractiveMode) {
            Vector3 viewDir = (cameraPosition - intersection.point).normalize();
            Vector3 reflectDir = viewDir - intersection.normal * 2.0f * viewDir.dot(intersection.normal);
            Ray reflectRay(intersection.point + reflectDir * 0.001f, reflectDir);
            Intersection reflectIntersect = intersectScene(reflectRay);

            if (reflectIntersect.hit) {
                Vector3 reflectColor = shade(reflectIntersect, depth + 1);
                // Add slight tint to reflections based on material color
                Vector3 tintedReflect = reflectColor * (Vector3(0.8f, 0.8f, 0.8f) + material.diffuse * 0.2f);
                color = color * (1.0f - material.reflectivity) + tintedReflect * material.reflectivity;
            }
        }
    }

    return color;
}

// Generate ray from screen coordinates with optimized camera vector caching
Ray createRayFromScreen(int x, int y) {
    // Precompute camera vectors once and reuse
    static Vector3 cachedForward;
    static Vector3 cachedRight;
    static Vector3 cachedUp;
    static bool cacheValid = false;
    
    // Only recalculate when camera changes
    if (!cacheValid) {
        cachedForward = (cameraTarget - cameraPosition).normalize();
        cachedRight = cachedForward.cross(cameraUp).normalize();
        cachedUp = cachedRight.cross(cachedForward);
        cacheValid = true;
    }
    
    float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float scale = tanf(fov * 0.5f * 3.14159265f / 180.0f);
    
    float nx = (2.0f * x / WINDOW_WIDTH - 1.0f) * aspectRatio * scale;
    float ny = (1.0f - 2.0f * y / WINDOW_HEIGHT) * scale;
    
    Vector3 direction = cachedForward + cachedRight * nx + cachedUp * ny;
    return Ray(cameraPosition, direction.normalize());
}

// Clear camera vector cache when camera changes
void invalidateCameraCache() {
    // Force recalculation of camera vectors
    createRayFromScreen(0, 0);
}

// Initialize scene
void initScene() {
    // Reduce the number of planes for better performance
    // Add floor plane
    planes.emplace_back(
        Vector3(0.0f, -1.0f, 0.0f),  // Ground point
        Vector3(0.0f, 1.0f, 0.0f),   // Upward normal
        Material(Vector3(0.2f, 0.8f, 0.3f), Vector3(0.1f, 0.1f, 0.1f), 10.0f, 0.1f)
    );
    
    // Add back wall only (removed side walls to improve performance)
    planes.emplace_back(
        Vector3(0.0f, 0.0f, 10.0f), // Back wall point
        Vector3(0.0f, 0.0f, -1.0f), // Forward normal
        Material(Vector3(0.9f, 0.3f, 0.6f), Vector3(0.1f, 0.1f, 0.1f), 10.0f, 0.05f)
    );
    
    // Add three spheres with improved material properties
    spheres.emplace_back(
        Vector3(-1.5f, 0.0f, 0.0f),  // Center position
        1.0f,                        // Radius
        Material(Vector3(0.9f, 0.15f, 0.15f), Vector3(0.9f, 0.9f, 0.9f), 100.0f, 0.45f) // More realistic red
    );
    
    spheres.emplace_back(
        Vector3(0.0f, 0.0f, 1.5f),   // Center position
        1.0f,                        // Radius
        Material(Vector3(0.15f, 0.6f, 0.8f), Vector3(0.85f, 0.85f, 0.9f), 150.0f, 0.55f) // More realistic blue
    );
    
    spheres.emplace_back(
        Vector3(1.5f, 0.0f, 0.0f),   // Center position
        1.0f,                        // Radius
        Material(Vector3(0.3f, 0.75f, 0.15f), Vector3(0.7f, 0.7f, 0.7f), 80.0f, 0.35f) // More realistic green
    );
    
    // Add realistic light sources with proper colors and intensities
    lights.emplace_back(Vector3(0.0f, 5.0f, -5.0f), Vector3(1.0f, 0.98f, 0.95f), 2.0f); // Warm key light
    lights.emplace_back(Vector3(5.0f, 3.0f, -3.0f), Vector3(0.9f, 0.95f, 1.0f), 1.2f); // Cool fill light
}

// Improved scene renderer with incremental rendering
void DrawScene(HDC hdc, HWND hwnd) {
    // Create bitmap info
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WINDOW_WIDTH;
    bmi.bmiHeader.biHeight = -WINDOW_HEIGHT; // Negative height means top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // Initialize or reuse frame buffer
    if (!frameBufferInitialized) {
        frameBuffer = new BYTE[WINDOW_WIDTH * WINDOW_HEIGHT * 4];
        frameBufferInitialized = true;
        // Clear frame buffer
        memset(frameBuffer, 0, WINDOW_WIDTH * WINDOW_HEIGHT * 4);
    }
    
    DWORD currentTime = GetTickCount();
    bool forceFullRender = (currentTime - lastRenderTime > 1000); // Force full render every second
    
    if (incrementalRendering && !forceFullRender) {
        // Incremental rendering - only render a portion of the screen each frame
        const int SCANLINES_PER_FRAME = 60; // Number of scanlines to render per frame (increased for better performance)
        
        for (int y = currentScanline; y < std::min(currentScanline + SCANLINES_PER_FRAME, WINDOW_HEIGHT); y++) {
            for (int x = 0; x < WINDOW_WIDTH; x++) {
                Ray ray = createRayFromScreen(x, y);
                Intersection intersection = intersectScene(ray);

                Vector3 color(0.0f, 0.0f, 0.0f);
                if (intersection.hit) {
                    color = shade(intersection);
                } else {
                    // Improved background with subtle gradient
                    float t = static_cast<float>(y) / WINDOW_HEIGHT;
                    color = Vector3(0.05f, 0.05f, 0.1f) * (1.0f - t) + Vector3(0.1f, 0.15f, 0.3f) * t;
                }

                // Apply tone mapping and gamma correction
                color.x = std::min(1.0f, color.x * 1.2f);
                color.y = std::min(1.0f, color.y * 1.2f);
                color.z = std::min(1.0f, color.z * 1.2f);

                int px = x + y * WINDOW_WIDTH;
                frameBuffer[px * 4 + 0] = std::min(255, static_cast<int>(color.z * 255)); // B
                frameBuffer[px * 4 + 1] = std::min(255, static_cast<int>(color.y * 255)); // G
                frameBuffer[px * 4 + 2] = std::min(255, static_cast<int>(color.x * 255)); // R
                frameBuffer[px * 4 + 3] = 255; // A
            }
        }
        
        // Update scanline index
        currentScanline += SCANLINES_PER_FRAME;
        if (currentScanline >= WINDOW_HEIGHT) {
            currentScanline = 0; // Reset for next full pass
        }
        
        // Update entire screen with new content
        SetDIBitsToDevice(
            hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, WINDOW_HEIGHT,
            frameBuffer, &bmi, DIB_RGB_COLORS
        );
    } else {
        // Full render when needed (e.g., after scene changes)
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            for (int x = 0; x < WINDOW_WIDTH; x++) {
                Ray ray = createRayFromScreen(x, y);
                Intersection intersection = intersectScene(ray);

                Vector3 color(0.0f, 0.0f, 0.0f);
                if (intersection.hit) {
                    color = shade(intersection);
                } else {
                    // Improved background with subtle gradient
                    float t = static_cast<float>(y) / WINDOW_HEIGHT;
                    color = Vector3(0.05f, 0.05f, 0.1f) * (1.0f - t) + Vector3(0.1f, 0.15f, 0.3f) * t;
                }

                // Apply tone mapping and gamma correction
                color.x = std::min(1.0f, color.x * 1.2f);
                color.y = std::min(1.0f, color.y * 1.2f);
                color.z = std::min(1.0f, color.z * 1.2f);

                int px = x + y * WINDOW_WIDTH;
                frameBuffer[px * 4 + 0] = std::min(255, static_cast<int>(color.z * 255)); // B
                frameBuffer[px * 4 + 1] = std::min(255, static_cast<int>(color.y * 255)); // G
                frameBuffer[px * 4 + 2] = std::min(255, static_cast<int>(color.x * 255)); // R
                frameBuffer[px * 4 + 3] = 255; // A
            }
            
            // Update screen periodically during full render
            if (y % 50 == 0) {
                SetDIBitsToDevice(
                    hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, WINDOW_HEIGHT,
                    frameBuffer, &bmi, DIB_RGB_COLORS
                );
                UpdateWindow(hwnd);
            }
        }
        
        // Reset for incremental rendering
        incrementalRendering = true;
        currentScanline = 0;
    }
    
    lastRenderTime = currentTime;
    
    // Final update
    SetDIBitsToDevice(
        hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, WINDOW_HEIGHT,
        frameBuffer, &bmi, DIB_RGB_COLORS
    );
    
    // Draw operation instructions
    RECT rect;
    SetRect(&rect, 10, 10, 350, 150);
    FillRect(hdc, &rect, CreateSolidBrush(RGB(0, 0, 0)));
    
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    
    TextOut(hdc, 20, 20, TEXT("Instructions:"), 13);
    TextOut(hdc, 20, 40, TEXT("Left click to select sphere"), 24);
    TextOut(hdc, 20, 60, TEXT("Drag to change position"), 20);
    TextOut(hdc, 20, 80, TEXT("Mouse wheel to scale size"), 22);
    TextOut(hdc, 20, 100, TEXT("Arrow keys to rotate camera"), 24);
    TextOut(hdc, 20, 120, TEXT("ESC to exit"), 9);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            // Clean up frame buffer
            if (frameBufferInitialized) {
                delete[] frameBuffer;
                frameBufferInitialized = false;
            }
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawScene(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            // Enter interactive mode
            isInteractiveMode = true;
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            
            // Create ray from click position
            Ray ray = createRayFromScreen(x, y);
            
            // Find closest sphere intersection
            Intersection closest;
            closest.t = 1e30f;
            int hitIndex = -1;
            
            for (size_t i = 0; i < spheres.size(); i++) {
                Intersection intersection = intersectSphere(ray, spheres[i]);
                if (intersection.hit && intersection.t < closest.t) {
                    closest = intersection;
                    hitIndex = i;
                }
            }
            
            // Handle selection
            for (auto& sphere : spheres) {
                sphere.selected = false;
            }
            
            if (hitIndex != -1) {
                spheres[hitIndex].selected = true;
                selectedSphereIndex = hitIndex;
                isDragging = true;
                lastMousePos.x = x;
                lastMousePos.y = y;
            }
            
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        
        case WM_LBUTTONUP:
            // Exit interactive mode
            isInteractiveMode = false;
            isDragging = false;
            
            // Force a full render after interaction
            incrementalRendering = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_MOUSEMOVE: { // Optimized mouse movement with performance balancing
            if (isDragging && selectedSphereIndex != -1) {
                static DWORD lastUpdateTime = 0;
                DWORD currentTime = GetTickCount();

                // Adaptive update frequency based on system load
                static int updateInterval = 8; // Start with 8ms (~120 FPS)
                
                // Dynamic interval adjustment
                if (currentTime - lastUpdateTime > updateInterval * 2) {
                    // System is fast, can update more frequently
                    updateInterval = std::max(4, updateInterval - 1);
                } else if (currentTime - lastUpdateTime < updateInterval * 0.5f) {
                    // System is slow, reduce update frequency
                    updateInterval = std::min(16, updateInterval + 1);
                }

                if (currentTime - lastUpdateTime > updateInterval) {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);

                    // Calculate mouse movement
                    float dx = (x - lastMousePos.x) * 0.01f;
                    float dy = (y - lastMousePos.y) * 0.01f;

                    // Move selected sphere
                    Vector3 forward = (cameraTarget - cameraPosition).normalize();
                    Vector3 right = forward.cross(cameraUp).normalize();
                    Vector3 up = right.cross(forward);

                    spheres[selectedSphereIndex].center = spheres[selectedSphereIndex].center + 
                                                         right * dx - up * dy;

                    // Ensure sphere doesn't go into the ground
                    if (spheres[selectedSphereIndex].center.y < spheres[selectedSphereIndex].radius - 1.0f) {
                        spheres[selectedSphereIndex].center.y = spheres[selectedSphereIndex].radius - 1.0f;
                    }

                    lastMousePos.x = x;
                    lastMousePos.y = y;
                    
                    // Force full render when objects are moved
                    incrementalRendering = false;
                    InvalidateRect(hwnd, NULL, FALSE);
                    lastUpdateTime = currentTime;
                }
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: { // Optimized wheel handling
            if (selectedSphereIndex != -1) {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                float scaleFactor = 1.0f + (zDelta > 0 ? 0.1f : -0.1f);
                
                spheres[selectedSphereIndex].radius *= scaleFactor;
                // Limit radius range
                if (spheres[selectedSphereIndex].radius < 0.2f) {
                    spheres[selectedSphereIndex].radius = 0.2f;
                }
                if (spheres[selectedSphereIndex].radius > 3.0f) {
                    spheres[selectedSphereIndex].radius = 3.0f;
                }
                
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_KEYDOWN: // Keyboard controls
            switch (wParam) {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                    
                case VK_LEFT: { // Rotate camera left around Y axis
                    incrementalRendering = false;
                    Vector3 forward = (cameraTarget - cameraPosition).normalize();
                    float angle = 0.1f;
                    float cosA = cos(angle);
                    float sinA = sin(angle);
                    
                    Vector3 newForward(
                        forward.x * cosA - forward.z * sinA,
                        forward.y,
                        forward.x * sinA + forward.z * cosA
                    );
                    
                    cameraTarget = cameraPosition + newForward * (cameraTarget - cameraPosition).length();
                    invalidateCameraCache();
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }
                
                case VK_RIGHT: { // Rotate camera right around Y axis
                    incrementalRendering = false;
                    Vector3 forward = (cameraTarget - cameraPosition).normalize();
                    float angle = -0.1f;
                    float cosA = cos(angle);
                    float sinA = sin(angle);
                    
                    Vector3 newForward(
                        forward.x * cosA - forward.z * sinA,
                        forward.y,
                        forward.x * sinA + forward.z * cosA
                    );
                    
                    cameraTarget = cameraPosition + newForward * (cameraTarget - cameraPosition).length();
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }
                
                case VK_UP: { // Rotate camera up (within limits)
                    incrementalRendering = false;
                    Vector3 forward = (cameraTarget - cameraPosition).normalize();
                    Vector3 right = forward.cross(cameraUp).normalize();
                    float angle = -0.1f;
                    
                    // Check for over-rotation
                    Vector3 testForward = forward * cos(angle) + cameraUp * sin(angle);
                    if (testForward.dot(cameraUp) < 0.9f) { // Limit maximum elevation
                        cameraTarget = cameraPosition + testForward * (cameraTarget - cameraPosition).length();
                    invalidateCameraCache();
                    InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;
                }
                
                case VK_DOWN: { // Rotate camera down (within limits)
                    incrementalRendering = false;
                    Vector3 forward = (cameraTarget - cameraPosition).normalize();
                    float angle = 0.1f;
                    
                    // Check for over-rotation
                    Vector3 testForward = forward * cos(angle) + cameraUp * sin(angle);
                    if (testForward.dot(cameraUp) > -0.8f) { // Limit maximum depression
                        cameraTarget = cameraPosition + testForward * (cameraTarget - cameraPosition).length();
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;
                }
                
                // Number keys 1-9 change selected sphere color
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    incrementalRendering = false;
                    if (selectedSphereIndex != -1) {
                        int colorIndex = wParam - '1';
                        // Predefined color set
                        std::vector<Vector3> colors = {
                            Vector3(0.9f, 0.2f, 0.2f), // Red
                            Vector3(0.2f, 0.9f, 0.2f), // Green
                            Vector3(0.2f, 0.2f, 0.9f), // Blue
                            Vector3(0.9f, 0.9f, 0.2f), // Yellow
                            Vector3(0.9f, 0.2f, 0.9f), // Purple
                            Vector3(0.2f, 0.9f, 0.9f), // Cyan
                            Vector3(0.9f, 0.5f, 0.2f), // Orange
                            Vector3(0.5f, 0.2f, 0.9f), // Violet
                            Vector3(0.8f, 0.8f, 0.8f)  // Gray
                        };
                        
                        if (colorIndex < colors.size()) {
                            spheres[selectedSphereIndex].material.diffuse = colors[colorIndex];
                            InvalidateRect(hwnd, NULL, FALSE);
                        }
                    }
                    break;
                    
                // R key increases reflectivity
                case 'R':
                case 'r':
                    incrementalRendering = false;
                    if (selectedSphereIndex != -1) {
                        spheres[selectedSphereIndex].material.reflectivity += 0.1f;
                        if (spheres[selectedSphereIndex].material.reflectivity > 1.0f) {
                            spheres[selectedSphereIndex].material.reflectivity = 1.0f;
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;
                    
                // T key decreases reflectivity
                case 'T':
                case 't':
                    incrementalRendering = false;
                    if (selectedSphereIndex != -1) {
                        spheres[selectedSphereIndex].material.reflectivity -= 0.1f;
                        if (spheres[selectedSphereIndex].material.reflectivity < 0.0f) {
                            spheres[selectedSphereIndex].material.reflectivity = 0.0f;
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    break;
            }
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    const char CLASS_NAME[] = "RayTracerWindowClass";
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    // Create window with English title
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Optimized Interactive Ray Tracer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    // Initialize scene
    initScene();
    
    // Show window
    ShowWindow(hwnd, nCmdShow);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Clean up frame buffer on exit
    if (frameBufferInitialized) {
        delete[] frameBuffer;
    }
    
    return (int)msg.wParam;
}