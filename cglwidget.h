#ifndef CGLWIDGET_H
#define CGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMap>
#include "utils.h"

struct sBufferWithColor {
    int mode;
    QOpenGLBuffer vbo;
    QVector4D lineColor;
};

struct sPlane {
    sPlane(QVector3D _n, QVector3D _p): n(_n), p(_p){}
    QVector3D n; //plane normal
    QVector3D p; //point on plane
};

class cGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit cGLWidget(QWidget *parent = nullptr);

    void setLightPos(const QVector3D lp);

    float scale(float from_min, float from_max, float to_min, float to_max, float val);

    QTimer *mTimer;

    QVector<GLfloat> mPoints;

    void updateProjection(bool perspectiveProjection);
    void topView();
    void leftView();
    void rightView();
    void frontView();
    QVector3D lightPos() const;

    void setLightColor(const QColor color);
    QColor lightColor();

    //int mCurrFrame=0;

    void addToVBO(const std::vector<GLfloat>& buffer, int mode=GL_LINES, QVector4D lineColor=QVector4D(1,1,1,1));
    void addToVBO(const QString& name, const std::vector<GLfloat>& buffer, int mode=GL_LINES, QVector4D lineColor=QVector4D(1,1,1,1));
    void clearVBOs();

    bool screenToWorld(QVector2D xy_ndc, int z_ndc, QVector3D &ray_world);

    void destroyVBO(const QString &name);
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent *me);
    void mouseReleaseEvent(QMouseEvent *me);

    void mouseMoveEvent(QMouseEvent *me);
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);

private:

    GLuint fboHandle;
    GLuint renderTex;

    QMap<int, bool> mKeyDown;

    bool mPerspectiveProjection = true;

    int mAngle = 0;

    QMap<QString, sBufferWithColor> mNamedVBOs;
    std::vector<sBufferWithColor> mVBOs;

    QOpenGLTexture *mTexture=nullptr;
    QOpenGLTexture *mTextureHeightNormal=nullptr;
    QOpenGLTexture *mTextureGrid=nullptr;

    int mCount;

    QOpenGLVertexArrayObject mVAO;

    bool mLeftMouseButtonPressed = false;
    int mLeftMouseButtonPressCoordX;
    int mLeftMouseButtonPressCoordY;

    GLuint mTextures[10];

    float mRotX;
    float mRotY;

    float mPos2dx;
    float mPos3dy;

    QOpenGLShaderProgram mShader;

    int mouseX, mouseY;

    int mSavedX = 0;
    int mSavedY = 0;

    QVector3D ray_nds;
    QVector4D ray_clip;

    QVector3D mEye;
    QVector3D mView;
    QVector3D mUp;

    QVector4D mPVertex;

    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mOrthoProjectionMatrix;

    float mX;
    float mY;
    float mZ;

    QMatrix4x4 mWorldTranslate;
    QMatrix4x4 mWorldRotate;

    void initShaders();
    void initTextures();
    void add(QVector<GLfloat> &b, QVector3D v, const QVector2D tc=QVector2D());
    void add2D(QVector<GLfloat> &b, const QVector2D& v);
    void add3D(QVector<GLfloat> &b, const QVector3D& v);
    void add3D(QVector<GLfloat> &b, const QVector<QVector3D> &vbuff);

    void addVertex(QVector<GLfloat> &b, QVector3D v);
    void cylinder(const float radius, const float length, const int sections);
    bool screenToWorld(int winx, int winy, int z_ndc, QVector3D &ray_world);
    float LinearizeDepth(float depth);
signals:
    void glInitialized();
    void leftMouseButtonPressed(QVector3D);
    void rightMouseButtonPressed(QVector3D);
    void mouseMove(QVector3D);
public slots:
    void on_mTimer_timeout();

};

#endif // CGLWIDGET_H
