#include "camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : 
    Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), 
    MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, 
    float upX, float upY, float upZ, float yaw, float pitch) : 
    Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), 
    MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::getProjectionMatrix(ProjectionType type) {
    if (type == PERSPECTIVE)
        return glm::perspective(glm::radians(Zoom), aspect, zNear, zFar);
    else
        return glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 100.0f);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    glm::vec3 position = Position;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, 
    GLboolean constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f; 
}

void Camera::updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up    = glm::normalize(glm::cross(Right, Front));

    if (!shouldUseRadar) updateFrustum();
}

void Camera::updateFrustum() {
    const float halfVSide = zFar * tanf(fovY * 0.5f);
    const float halfHSide = halfVSide * aspect;
    const glm::vec3 frontMultFar = zFar * Front;

    frustum.allPlanes = {
        { Position + Front * zNear, Front },
        { Position + frontMultFar, -Front },
        { Position, glm::cross(Up, frontMultFar + Right * halfHSide) },
        { Position, glm::cross(frontMultFar - Right * halfHSide, Up) },
        { Position, glm::cross(Right, frontMultFar - Up * halfVSide) },
        { Position, glm::cross(frontMultFar + Up * halfVSide, Right) }
    };
}

bool Camera::radarInsideFrustum(glm::vec4& maxPoint, glm::vec4& minPoint) {
    glm::vec3 currentPoint(maxPoint.x, maxPoint.y, maxPoint.z);

    glm::vec3 v = currentPoint - Position;
    float pZ = glm::dot(v, Front);
    if (pZ < zNear || pZ > zFar) return false;

    float pY = glm::dot(v, Up);
    float height = pZ * 2.0f * tanf(fovY / 2);
    if (pY < -height / 2 || pY > height / 2) return false;

    float pX = glm::dot(v, Right);
    float width = height * aspect;
    if (pX < -width / 2 || pX > width / 2) return false;

    return true;
}

bool Camera::isInsideFrustum(glm::vec4& maxPoint, glm::vec4& minPoint) {
    if (shouldUseRadar) return radarInsideFrustum(maxPoint, minPoint);

    return frustum.isInside(maxPoint, minPoint);
}

bool Frustum::isInside(glm::vec4& maxPoint, glm::vec4& minPoint) {
    for (FrustumPlane& plane : allPlanes) {
        glm::vec3 positive(minPoint.x, minPoint.y, minPoint.z);
        if (plane.normal.x > 0.0f) positive.x = maxPoint.x;
        if (plane.normal.y > 0.0f) positive.y = maxPoint.y;
        if (plane.normal.z > 0.0f) positive.z = maxPoint.z;

        glm::vec3 negative(maxPoint.x, maxPoint.y, maxPoint.z);
        if (plane.normal.x < 0.0f) negative.x = minPoint.x;
        if (plane.normal.y < 0.0f) negative.y = minPoint.y;
        if (plane.normal.z < 0.0f) negative.z = minPoint.z;

        glm::vec3 direction = positive - plane.point;
        float distance = glm::dot(plane.normal, direction);

        if (distance < 0.0f) {
            return false;
        }

        direction = negative - plane.point;
        distance = glm::dot(plane.normal, direction);
        if (distance < 0.0f) {
            return false;
        }
    }

    return true;
}