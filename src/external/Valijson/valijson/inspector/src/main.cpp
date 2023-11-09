#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}
