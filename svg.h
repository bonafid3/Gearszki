#ifndef SVG_H
#define SVG_H

#include <QChar>
#include <QVector>
#include <QVector2D>

#include <vector>
#include <QtOpenGL>
#include "bounds2d.h"

class cSVGCmd
{
public:
    cSVGCmd(){}
    void set(QChar cmd) { this->cmd = cmd; }
    void set(QChar cmd, float op0) { set(cmd); ops.push_back(op0); }
    void set(QChar cmd, float op0, float op1) { set(cmd, op0); ops.push_back(op1); }
    void add(QString val) { ops.push_back(val.toFloat()); }

    QVector2D absStart;

    QChar cmd;
    QVector<float> ops;
};

class CharPolygon : public std::vector<QVector2D>
{
public:
    CharPolygon() {}
    Bounds2D mBounds;
};

class Character : public std::vector<CharPolygon>
{
public:
    Character() {}
    float width() { return mBounds.minX + mBounds.maxX; }
    float height() { return mBounds.maxY - mBounds.minY; }
    Bounds2D mBounds;
};

class SVG
{
public:
    SVG();

    void readChars();

    void processSVG(int index);

    float mX=0, mY=0;
    QVector<cSVGCmd> mCommands;

    size_t mSlot;
    QMap<int, Character> mPoints;

    int processCMD(cSVGCmd cmd);
    void parseCommands(int chr=65);
    bool isNum(QChar c);
    QVector2D bezier(QVector2D s, QVector2D e, QVector2D c0, QVector2D c1, const float f);
    void append(int chr, float x, float y);
    void append(int chr, QVector2D p);
    std::vector<GLfloat> glFloatArray(int chr, QVector2D offset);
private:
    void calcBounds(int chr);
    void translate(int chr, QVector2D v);
    void flipY(int chr);
    void scale(int chr, float s);
};

#endif // SVG_H
