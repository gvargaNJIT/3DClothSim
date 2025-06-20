#include <QApplication>
#include <QMainWindow>
#include <QKeyEvent>
#include "clothwidget.h"

class ClothWindow : public QMainWindow {
public:
    ClothWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Cloth Simulation");
        clothWidget = new ClothWidget(this);
        setCentralWidget(clothWidget);
        resize(800, 600);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape) {
            close();
        }
        QMainWindow::keyPressEvent(event);
    }

private:
    ClothWidget* clothWidget;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ClothWindow window;
    window.show();
    return app.exec();
}