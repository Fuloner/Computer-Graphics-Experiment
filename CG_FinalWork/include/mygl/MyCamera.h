/// @brief 第一人称自由摄像机类
/// <summary>
/// 支持WASD键盘移动、鼠标旋转视角、滚轮缩放。
/// 使用欧拉角（俯仰角Pitch / 偏航角Yaw）控制朝向，glm::lookAt生成视图矩阵。
/// </summary>
#ifndef MYCAMERA_H
#define MYCAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 摄像机移动方向的枚举
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// 摄像机参数的默认值
const float YAW         = -90.0f;    // 偏航角初始值（看向-Z方向）
const float PITCH       =  0.0f;     // 俯仰角初始值（水平）
const float SPEED       =  4.0f;     // 移动速度
const float SENSITIVITY =  0.075f;   // 鼠标灵敏度
const float ZOOM        =  45.0f;    // 视野（FOV）

class MyCamera
{
public:
    // 摄像机在世界空间中的属性向量
    glm::vec3 Position;   // 摄像机位置
    glm::vec3 Front;      // 前向方向（单位向量）
    glm::vec3 Up;         // 上方向（单位向量）
    glm::vec3 Right;      // 右方向（单位向量）
    glm::vec3 WorldUp;    // 世界空间的上方向（通常为(0,1,0)）

    // 欧拉角 —— 控制摄像机的朝向
    float Yaw;            // 偏航角（绕世界Y轴旋转）
    float Pitch;          // 俯仰角（绕局部X轴旋转）

    // 摄像机可调参数
    float MovementSpeed;       // 键盘移动速度
    float MouseSensitivity;    // 鼠标旋转灵敏度
    float Zoom;                // 视野角度（FOV，度）

    /// <summary>
    /// 使用向量参数构造摄像机
    /// </summary>
    /// <param name="position">初始位置</param>
    /// <param name="up">世界空间上方向</param>
    /// <param name="yaw">初始偏航角（度）</param>
    /// <param name="pitch">初始俯仰角（度）</param>
    MyCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
             glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f),
             float yaw          = YAW,
             float pitch        = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        Position = position;
        WorldUp  = up;
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors(); // 根据欧拉角计算方向向量
    }

    /// <summary>
    /// 使用标量参数构造摄像机
    /// </summary>
    MyCamera(float posX, float posY, float posZ,
             float upX,  float upY,  float upZ,
             float yaw,  float pitch)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp  = glm::vec3(upX, upY, upZ);
        Yaw      = yaw;
        Pitch    = pitch;
        updateCameraVectors();
    }

    /// <summary>
    /// 返回当前摄像机的视图矩阵（用于传入着色器）
    /// </summary>
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    /// <summary>
    /// 处理键盘移动输入（WASD / QE升降）
    /// </summary>
    /// <param name="direction">移动方向枚举</param>
    /// <param name="deltaTime">上一帧耗时（秒），用于帧率无关的移动速度</param>
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += WorldUp * velocity;
        if (direction == DOWN)
            Position -= WorldUp * velocity;
    }

    /// <summary>
    /// 处理鼠标移动输入 —— 旋转摄像机朝向
    /// </summary>
    /// <param name="xoffset">鼠标在屏幕X方向的位移量</param>
    /// <param name="yoffset">鼠标在屏幕Y方向的位移量</param>
    /// <param name="constrainPitch">是否将俯仰角限制在[-89°, 89°]内，避免万向锁翻转</param>
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // 限制俯仰角范围，防止视角翻转（万向锁问题）
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // 欧拉角改变后，重新计算所有方向向量
        updateCameraVectors();
    }

    /// <summary>
    /// 处理鼠标滚轮输入 —— 调整视野（FOV）实现缩放效果
    /// </summary>
    /// <param name="yoffset">滚轮滚动量，正值缩小FOV（放大），负值扩大FOV（缩小）</param>
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;    // 最小FOV，防止极端值
        if (Zoom > 60.0f)
            Zoom = 60.0f;   // 最大FOV
    }

private:
    /// <summary>
    /// 根据当前的偏航角(Yaw)和俯仰角(Pitch)重新计算Front、Right、Up向量
    /// 每当欧拉角改变后必须调用此函数
    /// </summary>
    void updateCameraVectors()
    {
        // 用欧拉角计算前向向量（球坐标 → 笛卡尔坐标）
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        // 由前向量与世界上方向叉积得到右向量，再叉积得到正交的上向量
        // 这保证了三个方向始终两两垂直
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
