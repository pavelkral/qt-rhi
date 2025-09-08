#ifndef CAMERA_H
#define CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

// Definuje možné směry pohybu kamery
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Výchozí hodnoty kamery
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;

class Camera
{
public:

    QVector3D Position;
    QVector3D Front;
    QVector3D Up;
    QVector3D Right;
    QVector3D WorldUp;
    // Eulerov
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;

    // Konstruktor
    Camera(QVector3D position = QVector3D(0.0f, 0.0f, 0.0f), QVector3D up = QVector3D(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
        : Front(QVector3D(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Vrací view matici vypočítanou pomocí Eulerových úhlů a LookAt matice
    QMatrix4x4 GetViewMatrix()
    {
        QMatrix4x4 view;
        view.lookAt(Position, Position + Front, Up);
        return view;
    }

    // Zpracuje vstup z klávesnice
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
    }

    // Zpracuje pohyb myši
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Omezení úhlu Pitch, aby se kamera nepřetočila
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Aktualizace vektorů Front, Right a Up
        updateCameraVectors();
    }

private:
    // Přepočítá vektory kamery z aktualizovaných Eulerových úhlů
    void updateCameraVectors()
    {
        // Vypočítá nový vektor Front
        QVector3D front;
        front.setX(cos(qDegreesToRadians(Yaw)) * cos(qDegreesToRadians(Pitch)));
        front.setY(sin(qDegreesToRadians(Pitch)));
        front.setZ(sin(qDegreesToRadians(Yaw)) * cos(qDegreesToRadians(Pitch)));
        Front = front.normalized();
        // Přepočítá vektor Right a Up
        Right = QVector3D::crossProduct(Front, WorldUp).normalized();
        Up    = QVector3D::crossProduct(Right, Front).normalized();
    }
};
#endif // CAMERA_H
