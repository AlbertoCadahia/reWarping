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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

#include <QMatrix4x4>
#include <QtMath>
#include <QVector3D>
#include <QJsonArray>
#include <QImage>
#include <QColor>

#include "tiffio.h"
#include "vertex.h"

#define GRIDSIZE 362


class QOpenGLShaderProgram;

class MainWindow :public QOpenGLWindow,
        protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();

    void createShaderProgram();//shaders
    void createBuffer();//VBO
    void createVAO();//VAO
    void updateVAO();//VAO
    void createFBO();//FBO

    void initElements();//Matrixes
    void renderCube();//render one cube
    void renderGrid();//render the grid
    void renderToTexture();
    void renderToScreen();

    void loadColorImage(QString filename);//load color image and store in 2D texture
    void loadDepthImage(QString filename);//load depth image and store in 2D texture
    void saveImages(QString colorFilename,QString alphaFilename);
    void loadJSON(QString jsonFilename);//loads a JSON file given name
    void createGrid(QVector3D posVector,QVector3D lookAtVector,QVector3D upVector);//creates the "pixels" over which the geometry shader will create the mesh

    QVector3D qArrayToqVector(QJsonValue arr);


protected slots:
    void teardownGL();
    void update();

private:
     // OpenGL State Information
     QOpenGLBuffer vbo;
     QOpenGLVertexArrayObject vao;
     QOpenGLShaderProgram *program;
     //FBO
     QOpenGLFramebufferObject *fbo;

     // Shader Information
     int uModelViewProj;//identificator for the uniform of the modelViewProjection matrix
     int uColorTex;//identificator for the uniform of the color texture
     int uDepthTex;//identificator for the uniform of the depth texture
     int uRenderTex;//identificator for the uniform of the render texture
     int uEye;//identificator to the position of the eye
     int uGridRight;//identificator for the uniform of the grid generation vectors
     int uGridUp;
     int uScreenSize;//identificator for the uniform that stores the inverse of the size of the grid
     int uGridSize;

     //Matrix
     QMatrix4x4 projection;
     QMatrix4x4 viewProjection;

     ////////////////////////////////////////////
     //Extract original pos, lookat and up from the JSON
     QVector3D originalPos;
     QVector3D originalLookAt;
     QVector3D originalUp;
     QVector3D finalPos;
     QVector3D finalLookAt;
     QVector3D finalUp;
     ////////////////////////////////////////////

     //Camera. Projection matrix
     float fov;
     float aspect;
     float nearPlane;
     float farPlane;

     //Image
     QImage  imageColorObject;
     TIFF*  imageDepthObject;

     //QImage  imageColorOutput;

     //Textures
     QOpenGLTexture *colorTex;
     QOpenGLTexture *depthTex;

     //Grid
     Vertex gridPixels[GRIDSIZE*GRIDSIZE];

     //JSON
     QJsonArray jsonArray;
     QJsonValue val;

     // Private Helpers
     void printVersionInformation();
     float angle=-30.0f;

     bool finish=false;
     bool rewarping=false;
     int index=0;
     int index2=0;
     int index3=0;

     int lap=0;

};

#endif // MAINWINDOW_H
