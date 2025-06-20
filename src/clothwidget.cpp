#include "clothwidget.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QTime>
#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>
#include <QPainter>

extern bool gravityEnabled;

ClothWidget::ClothWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      cloth(20, 20, 0.1f, 50.0f, 20.0f),
      cameraPos(1.0f, 1.0f, 3.0f),
      cameraTarget(1.0f, 1.0f, 0.0f),
      cameraUp(0.0f, 1.0f, 0.0f),
      mousePressed(false),
      windEnabled(false),
      windPending(false),
      lastFrameTime(QTime::currentTime().msecsSinceStartOfDay() / 1000.0f)
{
    setFocusPolicy(Qt::StrongFocus);
    timer.setInterval(16);
    connect(&timer, &QTimer::timeout, this, &ClothWidget::updateSimulation);
    timer.start();
}

ClothWidget::~ClothWidget() {}

void ClothWidget::initializeGL() {
    QOpenGLFunctions_3_3_Core* coreFuncs =
        QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(QOpenGLContext::currentContext());

    if (!coreFuncs) {
        qFatal("Failed to obtain OpenGL 3.3 core functions");
    }

    coreFuncs->initializeOpenGLFunctions();
    renderer.initialize(coreFuncs);

    glEnable(GL_DEPTH_TEST);
}

void ClothWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void ClothWidget::paintGL() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(width()) / height(), 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.drawText(10, 20, "WASD = Move Camera | QE = Zoom | F = Wind | Mouse = Grab | R = Reset");
    painter.drawText(10, 40, "1-4 = Shading Modes: 1=Basic 2=Enhanced 3=Height 4=Fresnel");
    painter.end();

    renderer.render(cloth, cloth.getParticles(), projection, view);
}

void ClothWidget::updateSimulation() {
    float currentTime = QTime::currentTime().msecsSinceStartOfDay() / 1000.0f;
    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    if (mousePressed) {
        cloth.applymouseconstraint(mousePos, true);
    }

    if (windEnabled && mousePressed) {
        cloth.applywind(deltaTime);
    }

    cloth.update(deltaTime);

    update();
}

void ClothWidget::keyPressEvent(QKeyEvent *event) {
    float speed = 2.5f;
    switch (event->key()) {
        case Qt::Key_W:
            cameraPos += speed * glm::vec3(0.0f, 0.1f, 0.0f);
            break;
        case Qt::Key_S:
            cameraPos -= speed * glm::vec3(0.0f, 0.1f, 0.0f);
            break;
        case Qt::Key_A:
            cameraPos -= speed * glm::vec3(0.1f, 0.0f, 0.0f);
            break;
        case Qt::Key_D:
            cameraPos += speed * glm::vec3(0.1f, 0.0f, 0.0f);
            break;
        case Qt::Key_Q:
            cameraPos -= speed * glm::vec3(0.0f, 0.0f, 0.1f);
            break;
        case Qt::Key_E:
            cameraPos += speed * glm::vec3(0.0f, 0.0f, 0.1f);
            break;
        case Qt::Key_F:
            windEnabled = !windEnabled;
            break;
        case Qt::Key_R:
            cloth.reset();
            gravityEnabled = false;
            windEnabled = false;
            windPending = false;
            cameraPos = glm::vec3(1.0f, 1.0f, 3.0f);
            cameraTarget = glm::vec3(1.0f, 1.0f, 0.0f);
            cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
            break;
        case Qt::Key_1:
            renderer.setShadingMode(0);
            break;
        case Qt::Key_2:
            renderer.setShadingMode(1);
            break;
        case Qt::Key_3:
            renderer.setShadingMode(2);
            break;
        case Qt::Key_4:
            renderer.setShadingMode(3);
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

void ClothWidget::mousePressEvent(QMouseEvent *event) {
        mousePressed = true;
        gravityEnabled = true;
}

void ClothWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed = false;
    }
}

void ClothWidget::mouseMoveEvent(QMouseEvent *event) {
    mousePos.x = static_cast<float>(event->pos().x());
    mousePos.y = static_cast<float>(event->pos().y());
}
