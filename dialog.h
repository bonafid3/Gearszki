#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QVector2D>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>

#include "svg.h"

#include "clipper.h"

#include "cglwidget.h"

#define M 100000.0f

using namespace ClipperLib;

struct sPoint : public QVector2D
{
    sPoint(QVector2D p, float a=0.f) : QVector2D(p), angle(a) {}
    float angle;
    float segLen = 0.f;
};

struct Seg2f {
    QVector2D p0, p1;
    Seg2f(const QVector2D&  p0, const QVector2D& p1) : p0(p0), p1(p1) {}
};

struct PolySegs : public std::vector<Seg2f>
{
    void calcBounds()
    {
        mBounds.init();
        for(auto& p : *this) {
            mBounds.add(p.p0);
            mBounds.add(p.p1);
        }
    }

    std::vector<GLfloat> glFloatArray(float z = 0.0f)
    {
        std::vector<GLfloat> res;
        for(size_t i=0; i<size(); i++) {
            res.push_back(at(i).p0.x());
            res.push_back(at(i).p0.y());
            res.push_back(z);
            res.push_back(at(i).p1.x());
            res.push_back(at(i).p1.y());
            res.push_back(z);
        }
        return res;
    }

    void translate(QVector2D tv)
    {
        for(auto& v : *this) {
            v.p0 += tv;
            v.p1 += tv;
        }
    }

    PolySegs translated(QVector2D tv)
    {
        PolySegs res(*this);
        res.translate(tv);
        return res;
    }

    void rotate(float rad)
    {
        for(auto& v : *this) {
            float si = sin(rad);
            float co = cos(rad);
            float tx = v.p0.x();
            float ty = v.p0.y();
            v.p0.setX((co * tx) - (si * ty));
            v.p0.setY((si * tx) + (co * ty));
            tx = v.p1.x();
            ty = v.p1.y();
            v.p1.setX((co * tx) - (si * ty));
            v.p1.setY((si * tx) + (co * ty));
        }
    }

    PolySegs rotated(float rad)
    {
        PolySegs res(*this);
        res.rotate(rad);
        return res;
    }

    Bounds2D mBounds;
};

struct sPolygon : public std::vector<sPoint>
{
    void calcBounds()
    {
        mBounds.init();
        for(auto& p : *this) {
            mBounds.add(p);
        }
    }

    std::vector<GLfloat> glFloatArray(float z=0.0f)
    {
        std::vector<GLfloat> res;
        for(size_t i=0; i<size(); i++)
        {
            size_t j=i+1; if(j==size()) j=0;
            res.push_back(at(i).x());
            res.push_back(at(i).y());
            res.push_back(z);
            res.push_back(at(j).x());
            res.push_back(at(j).y());
            res.push_back(z);
        }
        return res;
    }

    Path path()
    {
        Path res;
        for(auto& v : *this) {
            res << IntPoint(v.x()*M, v.y()*M);
        }
        return res;
    }

    sPolygon translated(QVector2D tv)
    {
        sPolygon res(*this);
        res.translate(tv);
        return res;
    }

    void translate(QVector2D tv)
    {
        for(auto& v: *this)
            v += tv;
    }

    void rotate(float rad)
    {
        for(auto& v: *this) {
            float si = sin(rad);
            float co = cos(rad);
            float tx = v.x();
            float ty = v.y();
            v.setX((co * tx) - (si * ty));
            v.setY((si * tx) + (co * ty));
        }
    }
    Bounds2D mBounds;
};

struct sSpline
{
    std::vector<sPoint> mPoints;

    float getT(float distance, float alpha, float tension) {
        for(size_t i=0;i<mPoints.size(); ++i) {
            if(distance - mPoints[i].segLen <= 0) {
                float t = i;
                QVector2D prev;
                for(; t<mPoints.size(); t+=0.0005f) {
                    QVector2D p = getSplinePoint(t, alpha, tension);
                    if(t>i) {
                        QLineF line(prev.x(), prev.y(), p.x(), p.y());
                        distance -= line.length();
                        if(distance <= 0.f) return t;
                    }
                    prev = p;
                }
            } else {
                distance -= mPoints[i].segLen;
            }
        }
        return -1.0;
    }

    float distance(QVector2D p0, QVector2D p1) {
        return (p0 - p1).length();
    }

    float calcTotalLen(float alpha, float tension) {
        for(size_t i=0; i<mPoints.size(); i++) mPoints[i].segLen = 0.f;

        float total = 0.f;
        QVector2D prev = getSplinePoint(0.f, alpha, tension);
        for(float t=0.0005f; t<mPoints.size(); t+=0.0005f) {
            QVector2D p = getSplinePoint(t, alpha, tension);
            QLineF line(prev.x(), prev.y(), p.x(), p.y());
            mPoints[static_cast<size_t>(t)].segLen += line.length();
            total += line.length();
            prev = p;
        }
        return total;
    }

    QVector2D doTheMath(float t, double alpha, double tension, bool gradient)
    {
        int p0i, p1i, p2i, p3i;

        p1i = (int)t;
        p2i = (p1i + 1) % mPoints.size();
        p3i = (p2i + 1) % mPoints.size();
        p0i = p1i >= 1 ? p1i - 1 : mPoints.size() - 1;

        QVector2D p0 = mPoints[p0i];
        QVector2D p1 = mPoints[p1i];
        QVector2D p2 = mPoints[p2i];
        QVector2D p3 = mPoints[p3i];

        t = t - (int)t;

        float t0 = 0.f;
        float t1 = t0 + qPow(distance(p0, p1), alpha);
        float t2 = t1 + qPow(distance(p1, p2), alpha);
        float t3 = t2 + qPow(distance(p2, p3), alpha);

        QVector2D m1 = (1.0f - tension) * (t2 - t1) *
            ((p1 - p0) / (t1 - t0) - (p2 - p0) / (t2 - t0) + (p2 - p1) / (t2 - t1));
        QVector2D m2 = (1.0f - tension) * (t2 - t1) *
            ((p2 - p1) / (t2 - t1) - (p3 - p1) / (t3 - t1) + (p3 - p2) / (t3 - t2));

        QVector2D a = 2.0f * (p1 - p2) + m1 + m2;
        QVector2D b = -3.0f * (p1 - p2) - m1 - m1 - m2;
        QVector2D c = m1;
        QVector2D d = p1;

        return  gradient ?
                    a * 3 * t * t +
                    b * 2 * t +
                    c
                :
                    a * t * t * t +
                    b * t * t +
                    c * t +
                    d;
    }

    QVector2D getSplinePoint(float t, float alpha, float tension)
    {
        return doTheMath(t, alpha, tension, false);
    }

    QVector2D getSplineGradient(float t, float alpha, float tension)
    {
        return doTheMath(t, alpha, tension, true);
    }
};

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

    float displayText(QString text, QVector2D pos);
    std::vector<GLfloat> compileText(const QString text, const QVector2D pos);
protected:
    bool eventFilter(QObject *o, QEvent *e);

private slots:

    void on_mAlpha_valueChanged(double arg1);
    void on_mTension_valueChanged(double arg1);

    void on_mGradientSlider_valueChanged(int value);

    void on_mTeeth_valueChanged(int arg1);
    void on_mSamples_valueChanged(int arg1);

    // from gl widget
    void onGlInitialized();
    void onMouseMove(QVector3D pos);
    void onLeftMouseButtonPressed(QVector3D p);
    void onRightMouseButtonPressed(QVector3D p);
    void on_mEditModeGroupBox_toggled(bool arg1);

    // camera control
    void on_mPerspectiveRadioButton_toggled(bool checked);
    void on_mFrontButton_clicked();
    void on_mLeftButton_clicked();
    void on_mTopButton_clicked();
    void on_mRightButton_clicked();

    void on_mGearGroupBox_toggled(bool arg1);

    void on_mCalculateButton_clicked();
    void on_mSaveButton_clicked();

private:
    Ui::Dialog *ui;
    sSpline mPath;

    sPolygon mSpline;
    sPolygon mGear;

    Path mGear2;

    float mP = 0.1f;
    float mD = 0.02f;
    float mPrevError=0.f;

    QVector2D mMousePos;

    SVG mSVG;

    void rotatePath(std::vector<IntPoint> &p, float angle);
    void translatePath(std::vector<IntPoint> &p, float x);

    QString DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2);
    void writeDXF(QString fname, Path poly);

    void drawSplines();
    sPoint calcAngleForPoint(QVector2D);
    float sample(float t);
    float sample2(float t);

    void addPoint(QVector2D p);
    void removePoint(QVector2D p);

    void drawScene();
    //void drawSpline(struct Polygon spline);
    float angleBetween(QVector2D v1, QVector2D v2);
    void drawVectors(float x, float y);
    float iterate(float diameter);
    float pd(float sp, float mv);
    std::vector<GLfloat> pathToGLfloatArray(Path &path);
    void drawSystem();
    void rebuildModel();
    void drawControlPoints();
    std::vector<sPoint>::iterator findMinDistPoint(QVector2D p);
    void drawGuides();
};
#endif // DIALOG_H
