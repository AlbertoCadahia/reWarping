/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
*                                                                             *
*  Made by  Alberto Cadahía Subiñas                                           *
*                                                                             *
*  Master in Computer Graphics, Videogames and Virtual Reality                *
*  Master's Thesis                                                            *
*                                                                             *
*                                                                             *
*  a.cadahia@hotmail.com                                                      *
*                                                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "mainwindow.h"
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 2);
    //format.setSamples(8);

    MainWindow* w = new MainWindow();
    w->setFormat(format);
    w->resize(362,362);
    w->show();

    return a.exec();
}
