#include "utils.h"

//#define qd qDebug()

float d2r(float deg)
{
    return deg * 0.0174533f;
}

float r2d(const float rad)
{
    return rad * 57.2958f;
}

QByteArray readFile(const QString fname)
{
    QFile f(fname);
    f.open(QFile::ReadOnly);
    QByteArray data = f.readAll();
    f.close();
    return data;
}

void writeFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::WriteOnly);
    f.write(data);
    f.close();
}

void appendFile(const QString fname, QByteArray data)
{
    QFile f(fname);
    f.open(QFile::Append);
    f.write(data);
    f.close();
}

float crossProduct(const QVector2D& v1, const QVector2D& v2) {
    return v1.x() * v2.y() - v1.y() * v2.x();
}
