#include "svg.h"
#include "utils.h"
#include <QByteArray>

SVG::SVG()
{
}

void SVG::readChars()
{
    for(int i=0; i<255; i++) {
        processSVG(i);
    }
}

void SVG::processSVG(int index)
{
    //SVG interpreter
    QString fname(":/font/" + toStr(index) + ".svg");

    if(!QFile::exists(fname)) return;

    QByteArray data = readFile(fname);

    data.replace("\r","");
    data.replace("\n","");
    data.replace("\t","");

    QRegExp rx("<path.* d=\"([^\">]+)\"\\s?/>");
    rx.setMinimal(true); // important, make regexp lazy instead of greedy!
    int offset=0;
    while((offset = rx.indexIn(data,offset)) != -1) {
        QString path = rx.cap(1);
        qd << index << char(index) << path;

        mCommands.clear();

        int i=0;

        while(i < path.size()) {
            cSVGCmd cmd;

            // skip space if any
            if(path[i] == ' ') {
                i++;
                continue;
            }

            cmd.set(path[i]);
            int operands = processCMD(cmd);

            i++;

            if(operands == 0) {
                qd << "z command?";
            }

            if(operands == -1) {
                qd << "unknown";
                break;
            }

            for(int j=0; j<operands; j++) {
                QString val;

                if(path[i] == '-') {
                    val += "-";
                    i++;
                }

                if(path[i] == ',') i++;

                while(isNum(path[i]))
                    val += path[i++];

                if(val.size())
                    cmd.add(val);
            }

            mCommands.push_back(cmd);
        }

        // path parsed here
        parseCommands(index++);

        offset += rx.matchedLength();
    }

}


// get the number of operands for a command
int SVG::processCMD(cSVGCmd cmd)
{
    qd << "check cmd" << cmd.cmd;
    switch(cmd.cmd.unicode())
    {
    case 'M': return 2;

    case 'L':
    case 'l': return 2;

    case 'c':
    case 'C': return 6;

    case 'H':
    case 'h':
    case 'V':
    case 'v': return 1;

    case 's':
    case 'S': return 4;

    case 'z': return 0;
    case ' ': return 0;

    case ',': return 0;

    default:
        return -1;
    }
}

std::vector<GLfloat> SVG::glFloatArray(int chr, QVector2D offset)
{
    std::vector<GLfloat> res;
    for(size_t slot = 0; slot<mPoints[chr].size(); slot++)
    {
        for(size_t i=0; i<mPoints[chr][slot].size(); i++) {
            size_t j=i+1; if(j==mPoints[chr][slot].size()) j=0;
            QVector2D v1 = mPoints[chr][slot][i] + offset;
            QVector2D v2 = mPoints[chr][slot][j] + offset;
            res.push_back(v1.x());
            res.push_back(v1.y());
            res.push_back(0);
            res.push_back(v2.x());
            res.push_back(v2.y());
            res.push_back(0);
        }
    }
    return res;
}

void SVG::append(int chr, float x, float y)
{
    append(chr, QVector2D(x,y));
}

void SVG::append(int chr, QVector2D p)
{
    CharPolygon placeholder;
    if(mPoints[chr].size() == mSlot) mPoints[chr].push_back(placeholder);
    mPoints[chr][mSlot].push_back(p);
}

void SVG::parseCommands(int chr)
{
    mX = 0;
    mY = 0;
    mSlot = 0;

    for(int i=0; i<mCommands.size(); ++i) {
        cSVGCmd& cmd = mCommands[i];

        if(cmd.cmd == "c") {
            QVector2D s(mX, mY);

            cmd.absStart = s;

            QVector2D c0(cmd.ops[0], cmd.ops[1]); c0 += s;
            QVector2D c1(cmd.ops[2], cmd.ops[3]); c1 += s;
            QVector2D e(cmd.ops[4], cmd.ops[5]); e += s;

            QVector2D res = bezier(s, e, c0, c1, 0.5);

            append(chr, s);
            append(chr, res);
            append(chr, e);

            mX = e.x();
            mY = e.y();
        } else if(cmd.cmd == "C") {
            QVector2D s(mX, mY);

            cmd.absStart = s;

            QVector2D c0(cmd.ops[0], cmd.ops[1]);
            QVector2D c1(cmd.ops[2], cmd.ops[3]);
            QVector2D e(cmd.ops[4], cmd.ops[5]);

            QVector2D res = bezier(s, e, c0, c1, 0.5);
            mX = res.x();
            mY = res.y();

            append(chr, s);
            append(chr, res);
            append(chr, e);

            mX = e.x();
            mY = e.y();
        } else if(cmd.cmd == 's') {
            QVector2D s(mX, mY);
            cmd.absStart = s;
            QVector2D c0; // to be calculated from previous command operands
            QVector2D c1(cmd.ops[0], cmd.ops[1]); c1 += s;
            QVector2D e(cmd.ops[2], cmd.ops[3]); e += s;
            if(i > 0) {
                cSVGCmd& prevCmd = mCommands[i-1];
                if(prevCmd.cmd == 'c') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'C') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]);
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]);
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 's') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'S') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]);
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]);
                    c0 = pe + (pe - pcp1);
                } else {
                    qd << "CMD" << cmd.cmd << "prev" << prevCmd.cmd;
                    Q_ASSERT(false);
                }

                QVector2D res = bezier(s, e, c0, c1, 0.5);

                append(chr, s);
                append(chr, res);
                append(chr, e);

                mX = e.x();
                mY = e.y();
            }
        }
        else if(cmd.cmd == 'S') {
            QVector2D s(mX, mY);
            cmd.absStart = s;
            QVector2D c0; // to be calc
            QVector2D c1(cmd.ops[0], cmd.ops[1]);
            QVector2D e(cmd.ops[2], cmd.ops[3]);
            if(i > 0) {
                cSVGCmd& prevCmd = mCommands[i-1];
                if(prevCmd.cmd == 'c') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'C') {
                    QVector2D pe(prevCmd.ops[4], prevCmd.ops[5]);
                    QVector2D pcp1(prevCmd.ops[2], prevCmd.ops[3]);
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 's') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]); pe += prevCmd.absStart;
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]); pcp1 += prevCmd.absStart;
                    c0 = pe + (pe - pcp1);
                } else if(prevCmd.cmd == 'S') {
                    QVector2D pe(prevCmd.ops[2], prevCmd.ops[3]);
                    QVector2D pcp1(prevCmd.ops[0], prevCmd.ops[1]);
                    c0 = pe + (pe - pcp1);
                } else {
                    qd << "CMD" << cmd.cmd << "prev" << prevCmd.cmd;
                    Q_ASSERT(false);
                }

                QVector2D res = bezier(s, e, c0, c1, 0.5);

                append(chr, s);
                append(chr, res);
                append(chr, e);

                mX = e.x();
                mY = e.y();
            }
        } else if(cmd.cmd == 'M') {
            mX = cmd.ops[0];
            mY = cmd.ops[1];
            qd << mX;
            qd << mY;
            append(chr, mX, mY);
            continue;
        }
        else if(cmd.cmd == "z"){
            qd << "close path";
            mSlot++;
            continue;
        } else if(cmd.cmd == 'L') {
            mX = cmd.ops[0];
            mY = cmd.ops[1];
        } else if(cmd.cmd == 'H') {
            mX = cmd.ops[0];
        } else if(cmd.cmd == 'V') {
            mY = cmd.ops[0];
        } else if(cmd.cmd == 'l') {
            mX += cmd.ops[0];
            mY += cmd.ops[1];
        } else if(cmd.cmd == 'h') {
            mX += cmd.ops[0];
        } else if(cmd.cmd == 'v') {
            mY += cmd.ops[0];
        }

        // insert the calculated point
        qd << mX;
        qd << mY;

        append(chr, mX, mY);
    }

    flipY(chr);
    //scale(chr, 3.0f);
}

void SVG::flipY(int chr)
{
    for(size_t slot=0; slot<mPoints[chr].size(); slot++) {
        for(auto& p : mPoints[chr][slot]) {
            p.setY(-p.y());
        }
    }
    calcBounds(chr);
}

void SVG::scale(int chr, float s)
{
    for(size_t slot=0; slot<mPoints[chr].size(); slot++) {
        for(auto& p : mPoints[chr][slot]) {
            p *= s;
        }
    }
    calcBounds(chr);
}

void SVG::translate(int chr, QVector2D v)
{
    for(size_t slot=0; slot<mPoints[chr].size(); slot++) {
        for(auto& p : mPoints[chr][slot]) {
            p += v;
        }
    }
    calcBounds(chr);
}

// calculate char bounding box
void SVG::calcBounds(int chr)
{
    for(size_t slot=0; slot<mPoints[chr].size(); slot++) {
        mPoints[chr][slot].mBounds.init();

        for(auto p : mPoints[chr][slot]) {
            mPoints[chr][slot].mBounds.add(p);
        }

        if(chr==37)
            qd << mPoints[chr][slot].mBounds;

        if(slot==0) mPoints[chr].mBounds = mPoints[chr][slot].mBounds;
        else mPoints[chr].mBounds |= mPoints[chr][slot].mBounds;
    }
    qd << mPoints[chr].mBounds;
}

bool SVG::isNum(QChar c)
{
    if((c >= '0' && c<='9') || c=='.') return true;
    return false;
}

QVector2D SVG::bezier(QVector2D s, QVector2D e, QVector2D c0, QVector2D c1, const float f)
{
    QVector2D q0 =  s +  (c0 - s) * f;
    QVector2D q1 = c0 + (c1 - c0) * f;
    QVector2D q2 = c1 +  (e - c1) * f;
    QVector2D r0 = q0 + (q1 - q0) * f;
    QVector2D r1 = q1 + (q2 - q1) * f;
    return r0 + (r1 - r0) * f;
}

