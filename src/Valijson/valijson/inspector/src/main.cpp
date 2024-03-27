#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < 0x060000
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}
