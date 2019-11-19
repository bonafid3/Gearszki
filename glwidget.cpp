#include "glwidget.h"
#include <QDebug>
#include <QtOpenGL>

#include <qmath.h>

#include <locale.h>

#include <QString>
#include <QVectorIterator>

#include <QTimer>

#include "utils.h"

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    setMouseTracking(true);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //glClearColor(0.5f, 0.8f, 0.5f, 0.5f);
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);

    //qd << "Supported shading language version"  << QString((char*)glGetString(GL_VENDOR))<< QString((char*)glGetString(GL_RENDERER)) << QString((char*)glGetString(GL_VERSION)) /*<< QString((char*)glGetString(GL_EXTENSIONS))*/ << QString((char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    qDebug() << "Initializing shaders...";

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE); // qt surface format sets the number of sampling

//    initShaders();

    glClearDepth(1.0); // Depth Buffer Setup
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Testing To Do

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//    mVAO.create();
//    QOpenGLVertexArrayObject::Binder vaoBinder(&mVAO);
    glActiveTexture(GL_TEXTURE0);

    installEventFilter(this);

    //emit(glInitialized());
}

void GLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w)
    Q_UNUSED(h)
//    updateProjection(mPerspectiveProjection);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
}
