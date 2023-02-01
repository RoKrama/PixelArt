#include "PixelArt.h"
#include "InitArt.h"
#include "UserInterface.h"
#include <QtWidgets/QApplication>
#include <thread>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    UserInterface instance;
    return a.exec();
}
//auto t1 = std::chrono::high_resolution_clock::now();
//auto t2 = std::chrono::high_resolution_clock::now();
//qDebug() << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();