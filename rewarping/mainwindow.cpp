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

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <string>

#define PI 3.14159265359
#define DEGTORAD 0.01745329251 //PI/180



/***************************************************/
/*************CONSTRUCTOR & DESTRUCTOR**************/
/***************************************************/

MainWindow::MainWindow()
{

}

MainWindow::~MainWindow()
{
    teardownGL();
}

/***************************************************/
/**************QT OPENGL CORE METHODS***************/
/***************************************************/

void MainWindow::initializeGL()
{
    // Initialize OpenGL Backend
    initializeOpenGLFunctions();
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(teardownGL()), Qt::DirectConnection);
    connect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
    printVersionInformation();

    //initialize elements of the camera matrixes
    initElements();

    // Set global information
    makeCurrent();
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);

    // Application-specific initialization
    {
        // Create Shader (Do not release until VAO is created)
        createShaderProgram();

        // Store Uniform Locations
        uModelViewProj = program->uniformLocation("viewProj");
        uEye=program->uniformLocation("eye");
        uGridUp = program->uniformLocation("gridUp");
        uGridRight = program->uniformLocation("gridRight");
        uScreenSize = program->uniformLocation("screenSizeInverse");
        uGridSize= program->uniformLocation("gridSize");

        //Uniforms for the textures
        uColorTex = program->uniformLocation("colorTex");
        uDepthTex = program->uniformLocation("depthTex");

        // Create and link Vertex Array Object
        createVAO();

        //create FBO for rendering to texture
        createFBO();

        //loads the JSON document for extracting the elements (name of files and 'pos' 'lookat' and 'up' vectors)
        QString jsonFile("img/log.json");
        loadJSON(jsonFile);

    }
}

void MainWindow::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect = (float)width / (float)height;
    projection.setToIdentity();
    projection.perspective(fov, aspect, nearPlane, farPlane);
}

void MainWindow::paintGL()
{

    //Executed same times as number of origin renders (Initial view).
    if(!finish && !rewarping && (jsonArray.count()-1)>=0)
    {
        val= jsonArray[index].toObject().value("InitialPosLookatUp"+QString::number(index));

        //Extract original pos, lookat and up from the JSON
        originalPos= qArrayToqVector(val.toArray()[0]);
        originalLookAt= qArrayToqVector(val.toArray()[1]);
        originalUp= qArrayToqVector(val.toArray()[2]);

        //load initial Images for extracting the color and depth of every pixel
        QString colorFilename ("img/initialView/initialDiffuse"+ QString::number(index)+'_'+QString::number(index3)+ ".tiff");
        QString depthFilename ("img/depthView/initialDepth"+ QString::number(index)+'_'+QString::number(index3)+ ".tiff");
        loadColorImage(colorFilename);
        loadDepthImage(depthFilename);

        // Grid creation
        createGrid(originalPos,originalLookAt,originalUp);

        // Create Buffer VBO (Do not release until VAO is created)
        createBuffer();

        // update and link Vertex Array Object
        updateVAO();

    }

    //Executed same times as number of final renders. For each original render, this code is executed multiple times
    if(!finish && (jsonArray[index].toObject().length()-2)>=0)
    {
        val= jsonArray[index].toObject().value("finalPosLookatUp"+QString::number(index)+"_"+QString::number(index2));

        //Extract original pos, lookat and up from the JSON
        finalPos=qArrayToqVector(val.toArray()[0]);
        finalLookAt=qArrayToqVector(val.toArray()[1]);
        finalUp=qArrayToqVector(val.toArray()[2]);

        //Change the camera view acording to the JSON final positions
        QMatrix4x4 finalView;
        finalView.setToIdentity();
        finalView.lookAt(finalPos,finalLookAt,finalUp);

        //calculate the new modelViewProjection matrix to avoid doing in the server,

        viewProjection.setToIdentity();
        viewProjection=projection*finalView;


        renderToTexture();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderToScreen();
    }

    //Increase the indexes according to the lenght on the JSON arrays of number of initial renders and final renders.
    //Index2 is increased according to the  number of final renders
    if(index2<(jsonArray[index].toObject().length()-2))//Make sure dont access out of array bounds
    {
        index2++;
        rewarping=true;
    }
    //If final render index (index2) is increased, we don't want to increase the origin render index (index)
    else
    {
        if(index3<3)
        {
            index3++;
            index2=0;
            rewarping=false;

        }
        else{
            //Only increase index if all final renders corresponding to origin render are executed
            if(index<jsonArray.count()-1)//Make sure dont access out of array bounds
            {
                index++;
                index2=0;
                index3=0;
                rewarping=false;

            }
            //If reach both arrays end, set finish to true to not continue rendering nothing
            else
            {
                finish=true;
            }
        }

    }

    //Release VBO and VAO only every new grid creation
    if(!rewarping)
    {
        // Release (unbind) VBO and VAO
        vao.release();
        vbo.release();
    }
}

void MainWindow::update()
{

    // Schedule a redraw
    QOpenGLWindow::update();
}


/***************************************************/
/*******************SUB ROUTINES********************/
/***************************************************/

void MainWindow::initElements()
{
    fov=45.0f;
    nearPlane=1.0f;
    farPlane=1e6f;
    aspect=1.0f;

    //Projection matrix
    projection.setToIdentity();
    projection.perspective(fov, aspect, nearPlane, farPlane);
}

void MainWindow::renderToScreen()
{


    // Render using our shader
    program->bind();

    {//Render grid

        renderGrid();
    }

    //release VAO
    vao.release();

    //release program
    program->release();


}

void MainWindow::renderToTexture()
{
    fbo->bind();

    // Clear color and depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Render using our shaders
    program->bind();

    {//Render grid

        renderGrid();
    }

    //release VAO
    vao.release();

    //release program
    program->release();

    //Save the result of the rewarping in a new Image
    saveImages("img/result/rewarpedView/imageRewarped"+QString::number(index)+'_'+QString::number(index2)+"_"+QString::number(index3)+".tiff"
              , "img/result/alphaMask/imageAlphaMask"+QString::number(index)+'_'+QString::number(index2)+"_"+QString::number(index3)+".tiff");
    //release FBO
    fbo->release();

}

void MainWindow::createShaderProgram()
{
    //Create and link the shaders to the program
    program = new QOpenGLShaderProgram();
    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/simple.vert"))
        qDebug()<<"Vertex shader compilation failure";
    if(!program->addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/simple.geom"))
        qDebug()<<"Geometry shader compilation failure";
    if(!program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/simple.frag"))
        qDebug()<<"Fragment shader compilation failure";
    program->link();
    program->bind();
}

void MainWindow::createBuffer()
{
    vbo.create();
    vbo.bind();
    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo.allocate(gridPixels, sizeof(gridPixels));
}

void MainWindow::createVAO()//InitOBJ
{
    vao.create();
    vao.bind();
    program->enableAttributeArray(0);
    program->enableAttributeArray(1);
    program->enableAttributeArray(2);
    program->setAttributeBuffer(0, GL_FLOAT, Vertex::positionOffset(), Vertex::PositionTupleSize, Vertex::stride());
    program->setAttributeBuffer(1, GL_FLOAT, Vertex::colorOffset(), Vertex::ColorTupleSize, Vertex::stride());
    program->setAttributeBuffer(2, GL_FLOAT, Vertex::texCoordOffset(), Vertex::TexCoordTupleSize, Vertex::stride());

}

void MainWindow::updateVAO()
{
    vao.bind();
    program->enableAttributeArray(0);
    program->enableAttributeArray(1);
    program->enableAttributeArray(2);
    program->setAttributeBuffer(0, GL_FLOAT, Vertex::positionOffset(), Vertex::PositionTupleSize, Vertex::stride());
    program->setAttributeBuffer(1, GL_FLOAT, Vertex::colorOffset(), Vertex::ColorTupleSize, Vertex::stride());
    program->setAttributeBuffer(2, GL_FLOAT, Vertex::texCoordOffset(), Vertex::TexCoordTupleSize, Vertex::stride());

}

void MainWindow::createFBO()
{

    QOpenGLFramebufferObjectFormat *fboFormat= new QOpenGLFramebufferObjectFormat();
    //fboFormat->setSamples(8);
    fboFormat->setAttachment(QOpenGLFramebufferObject::Depth);

    fbo=new QOpenGLFramebufferObject(GRIDSIZE,GRIDSIZE,*fboFormat);
    bool status =fbo->isValid();
    qDebug()<<"FBO Status OK-> "<< status;

    bool binded=fbo->bind();
    qDebug()<<"FBO Binded-> "<< binded;

}

void MainWindow::createGrid(QVector3D eye,QVector3D center,QVector3D up)
{

    //Calculates the lower left corner in which the grid is going to be created. The grid fits the nearPlane

    up.normalize();
    float z=nearPlane+0.0001f;
    QVector3D lookAtDir=(center-eye).normalized();//calculates the direction of the lookat
    QVector3D gridCenter=eye+lookAtDir*z;//set the origin of the grid to a point displaced the nearPlane + a little offset.
    float h=tan(fov*0.5f*DEGTORAD)*z;//Calculates the height of the viewport. DEGTORAD=PI/180

    ///////////////////////////////////////////////////
    //Ortonormalization of UP
    up-=QVector3D::dotProduct(lookAtDir,up)*lookAtDir;
    up.normalize();
    ///////////////////////////////////////////////////

    QVector3D height=up*h;
    QVector3D right=QVector3D::crossProduct(lookAtDir,up).normalized();
    QVector3D width=right*(h*aspect);

    //Origin Point. Low left corner.
    QVector3D bottomLeftPixel=gridCenter-height-width;

    //Offset of the pixels
    QVector3D gridUp=2.0f*height/GRIDSIZE;//Divided by the number of pixels.
    QVector3D gridRight=2.0f*width/GRIDSIZE;

    //Inverse of the screen size to make posible normalize the texels in GPU
    QVector2D screenSizeInverse=QVector2D(1.0f/GRIDSIZE,1.0f/GRIDSIZE);

    program->bind();
    //Upload the uniforms to the GPU
    if(uEye!=-1)
        program->setUniformValue(uEye,eye);
    if(uGridUp!=-1)
        program->setUniformValue(uGridUp, gridUp*0.5f);
    if(uGridRight!=-1)
        program->setUniformValue(uGridRight, gridRight*0.5f);
    if(uScreenSize!=-1)
        program->setUniformValue(uScreenSize,screenSizeInverse);
    if(uGridSize!=-1)
        program->setUniformValue(uGridSize,GRIDSIZE-1.0f);
    program->release();

    //offset to point to the center of the first pixel
    bottomLeftPixel+=(gridUp*0.5f);
    bottomLeftPixel+=(gridRight*0.5f);


    for(int i=0;i<GRIDSIZE;i++)
    {
        for(int j=0;j<GRIDSIZE;j++)
        {//Increase of 0.5 texture coordinates because the grid is generated by the center of the pixels, not the quad vertexes.
            gridPixels[i*GRIDSIZE+j]=Vertex( bottomLeftPixel+(gridRight*j)+(gridUp*i), QVector3D( 1.0f, 1.0f, 1.0f),QVector2D((float)j+0.5f, (float)i+0.5f));
        }
    }

}

void MainWindow::renderGrid()
{
    //upload textures to the server, binding them to respective texture units
    if (uColorTex != -1)
        colorTex->bind(uColorTex);
    if (uDepthTex != -1)
        depthTex->bind(uDepthTex);

    //uploading the uniform values in the server
    if(uModelViewProj!=-1)
        program->setUniformValue(uModelViewProj, viewProjection);


    vao.bind();
    //Draws the arrays of elements
    glDrawArrays(GL_POINTS, 0, sizeof(gridPixels) / sizeof(gridPixels[0]));

}

/***************************************************/
/***********HELPERS AND AUXILIARY METHODS***********/
/***************************************************/


void MainWindow::saveImages(QString colorFilename, QString alphaFilename)
{

    //save the new image
    QImage fboColorImage(fbo->toImage());

    //extract alpha mask
    QImage fboAlphaImage=fboColorImage.createAlphaMask();

    fboColorImage=fboColorImage.convertToFormat(QImage::Format_Grayscale8);


    bool colorSaved=fboColorImage.save(colorFilename);
    bool alphaSaved=fboAlphaImage.save(alphaFilename);
    qDebug()<<"Image Color saved-> "<<colorSaved;
    qDebug()<<"Image Alpha saved-> "<<alphaSaved;

}

void MainWindow::loadColorImage(QString filename)
{
    //create a new QImage and load the image from file
    imageColorObject=QImage(filename);

    bool isNull=imageColorObject.isNull();
    qDebug()<<"Color Image loaded-> "<< !isNull;
    //apply vertical mirror modification
    imageColorObject=imageColorObject.mirrored(false,true);


    //extracts the color from the pixel gived an x and y index. Can be directly mapped to the triangles color.
    colorTex=new QOpenGLTexture(imageColorObject,QOpenGLTexture::DontGenerateMipMaps);
    //colorTex->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);

    qDebug()<< "ColorTex created-> "<< colorTex->isCreated();

}

void MainWindow::loadDepthImage(QString filename)
{
    //extracts the depth from the image and create a texture from it

    uint32 width, height;
    float *buf =NULL;

    //Convert from QString to char* to read the file
    QByteArray byteArray= filename.toLocal8Bit();
    char *file= byteArray.data();

    //Open the file for reading
    if((imageDepthObject = TIFFOpen(file, "r")) == NULL){
        qDebug()<< "Could not open Depth image";
        exit(0);
    }
    qDebug()<<"Depth Image loaded-> "<< true;


    // Find the width and height of the image
    TIFFGetField(imageDepthObject, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(imageDepthObject, TIFFTAG_IMAGELENGTH, &height);

    //Allocate memory in buffer of the line size
    buf = (float *)_TIFFmalloc(TIFFScanlineSize(imageDepthObject));
    if (buf == NULL){
        qDebug()<<"Could not allocate memory";
        exit(0);
    }

    //Create a float buffer to store the data from the depth image
    float * textureBuffer = new float[GRIDSIZE*GRIDSIZE];

    //Store the data in the buffer
    for(int i=0; i<height; i++)
    {
        TIFFReadScanline(imageDepthObject, buf, i);
        for(int j=0;j<width;j++)
        {
            //textureBuffer[(GRIDSIZE-1-i)*width+j]= buf[j];
            textureBuffer[i*width+j]= buf[j];
        }
    }

    //Free resources
    _TIFFfree(buf);
    TIFFClose(imageDepthObject);

    //Texture creating
    depthTex=new QOpenGLTexture(QOpenGLTexture::Target2D);
    depthTex->setSize(GRIDSIZE, GRIDSIZE);
    depthTex->setComparisonMode(QOpenGLTexture::CompareNone);
    depthTex->setFormat(QOpenGLTexture::R32F);
    depthTex->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
    depthTex->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, textureBuffer);
    depthTex->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);


    qDebug()<<"DepthTex created-> "<< depthTex->isCreated();
    delete textureBuffer;

}

void MainWindow::loadJSON(QString filename)
{
    //Create file and array to store the objects
    QFile file;
    QString values;
    //Open file for reading
    file.setFileName(filename);
    if(!file.open(QFile::ReadOnly)){
        qDebug()<< "Error, Cannot open the JSON file.";
        exit(1);
    }
    //Store all the file in the array
    values = file.readAll();
    file.close();

    //create and load a new JSON document from the values array
    QJsonDocument jsonDoc = QJsonDocument::fromJson(values.toUtf8());

    if(jsonDoc.isEmpty())
    {
        qDebug()<<"JSON Empty!";
        exit(1);
    }

    bool null=jsonDoc.isNull();
    qDebug()<<"JSON loaded-> "<<!null;
    //Store the json array  of all set of images in memory for forward accesing
    jsonArray= jsonDoc.array();
}

QVector3D MainWindow::qArrayToqVector(QJsonValue arr)
{
    QVector3D vec;
    vec.setX(arr.toArray()[0].toDouble());
    vec.setY(arr.toArray()[1].toDouble());
    vec.setZ(arr.toArray()[2].toDouble());

    return vec;
}

void MainWindow::teardownGL()
{
    // Release (unbind) shader program
    program->release();
    // Actually destroy our OpenGL information
    vao.destroy();
    vbo.destroy();
    //release textures
    colorTex->release();
    depthTex->release();
    //delete shaders program
    delete program;
}

void MainWindow::printVersionInformation()
{
    QString glType;
    QString glVersion;
    QString glProfile;
    ////
    // Get Version Information
    glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    // Get Profile Information
#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
    switch (format().profile())
    {
    CASE(NoProfile);
    CASE(CoreProfile);
    CASE(CompatibilityProfile);
    }
#undef CASE

    // qPrintable() will print our QString w/o quotes around it.
    qDebug() << qPrintable(glType) << qPrintable(glVersion) << "(" << qPrintable(glProfile) << ")";
}
