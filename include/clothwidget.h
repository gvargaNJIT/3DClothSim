#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QObject>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include "clothsim.h"
#include "openGL.h"

class ClothWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    ClothWidget(QWidget *parent = nullptr);
    ~ClothWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateSimulation();

private:
    QTimer timer;
    Cloth cloth;
    ClothRenderer renderer;

    glm::vec3 cameraPos;
    glm::vec3 cameraTarget;
    glm::vec3 cameraUp;

    glm::vec2 mousePos;
    bool mousePressed;
    bool windEnabled;
    bool windPending;

    float lastFrameTime;
    float deltaTime;
};