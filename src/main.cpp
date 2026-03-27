#include "app/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    llm::MainWindow window;
    window.show();
    return app.exec();
}
