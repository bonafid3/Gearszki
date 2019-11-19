#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QDebug>
#include <QByteArray>
#include <QVector2D>

#define qd qDebug()
#define ftoStr(x) QString::number(x, 'f', 2)
#define toStr(x) QString::number(x)

float d2r(float deg);

float r2d(const float rad);

QByteArray readFile(const QString fname);

void writeFile(const QString fname, QByteArray data);

void appendFile(const QString fname, QByteArray data);

float crossProduct(const QVector2D& v1, const QVector2D& v2);

#endif
