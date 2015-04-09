#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // it seems we need to setlocale to correctly parse the scores
    setlocale(LC_NUMERIC, "C");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
