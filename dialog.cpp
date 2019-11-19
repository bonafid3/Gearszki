#include "dialog.h"
#include "ui_dialog.h"

#include "utils.h"

#include <QSound>

using namespace ClipperLib;

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setWindowState(Qt::WindowMaximized);
    ui->mTableView->verticalHeader()->hide();

    ui->mFrictionDiscLabel->hide();

    ui->splitter->setStretchFactor(0, 5);
    ui->splitter->setStretchFactor(1, 1);

    connect(ui->mGLWidget, &cGLWidget::mouseMove, this, &Dialog::onMouseMove);

    QStandardItemModel *model = new QStandardItemModel(this);
    ui->mTableView->setModel(model);

    addPoint(QVector2D(0,-200));
    addPoint(QVector2D(0, 200));

    addPoint(QVector2D(50,0));
    addPoint(QVector2D(-50,0));

    ui->mGLWidget->setFocus();

    connect(ui->mGLWidget, &cGLWidget::leftMouseButtonPressed, this, &Dialog::onLeftMouseButtonPressed);
    connect(ui->mGLWidget, &cGLWidget::rightMouseButtonPressed, this, &Dialog::onRightMouseButtonPressed);

    connect(ui->mGLWidget, &cGLWidget::glInitialized, this, &Dialog::onGlInitialized);

    mSVG.readChars();

    drawScene();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::rebuildModel()
{
    QStandardItemModel *model = static_cast<QStandardItemModel*>(ui->mTableView->model());
    model->clear();

    QStringList hl; hl << "X coord" << "Y coord";
    model->setHorizontalHeaderLabels(hl);

    for(auto& pos : mPath.mPoints){
        int i = model->rowCount();
        model->setItem(i, 0, new QStandardItem(toStr(pos.x())));
        model->setItem(i, 1, new QStandardItem(toStr(pos.y())));

        ui->mTableView->resizeRowsToContents();
        ui->mTableView->scrollToBottom();
    }
}

void Dialog::onMouseMove(QVector3D pos)
{
    mMousePos = pos.toVector2D();
    drawGuides();
}

void Dialog::drawGuides()
{
    PolySegs lineToCursor, lineToClosest;
    lineToCursor.push_back(Seg2f(QVector2D(0,0), mMousePos));

    if(mPath.mPoints.size())
        lineToClosest.push_back(Seg2f(mMousePos, *findMinDistPoint(mMousePos)));
    else
        ui->mGLWidget->destroyVBO("linefToClosest");

    if(ui->mEditModeGroupBox->isChecked()) {
        ui->mGLWidget->addToVBO("coords", compileText(" X:" + ftoStr(mMousePos.x()) + ", Y:" + ftoStr(mMousePos.y()), mMousePos));
        ui->mGLWidget->addToVBO("lineToCursor", lineToCursor.glFloatArray());
        ui->mGLWidget->addToVBO("lineToClosest", lineToClosest.glFloatArray(), GL_LINES, QVector4D(1,0.8f,0,1));
    } else {
        ui->mGLWidget->destroyVBO("coords");
        ui->mGLWidget->destroyVBO("lineToCursor");
        ui->mGLWidget->destroyVBO("linefToClosest");
    }
}

void Dialog::addPoint(QVector2D p)
{
    if(mPath.mPoints.size()) {
        if((*findMinDistPoint(p) - p).length() == 0.f)
            return;
    }

    QSound* s = new QSound(":/click.wav", this);
    s->play();

    mPath.mPoints.push_back(calcAngleForPoint(p));
    std::sort(mPath.mPoints.begin(), mPath.mPoints.end(), [](sPoint v1, sPoint v2){
        return v1.angle < v2.angle;
    });
    drawGuides();
    rebuildModel();
}

std::vector<sPoint>::iterator Dialog::findMinDistPoint(QVector2D p)
{
    float minDist = std::numeric_limits<float>::max();
    std::vector<sPoint>::iterator res = mPath.mPoints.end(); // end means nothing found!
    for(auto it = mPath.mPoints.begin(); it!=mPath.mPoints.end(); ++it) {
        float dist = (p - *it).length();
        if(dist < minDist) {
            minDist = dist;
            res = it;
        }
    }
    return res;
}

void Dialog::removePoint(QVector2D p)
{
    if(mPath.mPoints.size()) {
        QSound* s = new QSound(":/sss.wav", this);
        s->play();
        mPath.mPoints.erase(findMinDistPoint(p));
        rebuildModel();
    }
    drawGuides();
}

void Dialog::drawScene()
{
    ui->mGLWidget->clearVBOs();
    drawSystem();
    drawControlPoints();
    if(mPath.mPoints.size() > 2) {
        drawSplines();
    }
}

void Dialog::onGlInitialized()
{
    qd << "gl init";
    drawScene();
}

void Dialog::onLeftMouseButtonPressed(QVector3D p)
{
    if(!ui->mEditModeGroupBox->isChecked())
        return;

    addPoint(p.toVector2D());
    drawScene();
}

void Dialog::onRightMouseButtonPressed(QVector3D p)
{
    if(!ui->mEditModeGroupBox->isChecked())
        return;

    removePoint(p.toVector2D());
    drawScene();
}

bool Dialog::eventFilter(QObject*o, QEvent* e) {
    return QDialog::eventFilter(o,e);
}

sPoint Dialog::calcAngleForPoint(QVector2D v)
{
    sPoint res(v,0);

    QVector2D v1(1, 0); // reference vector
    float det = v1.x()*v.y() - v1.y()*v.x();
    float dot = v1.x()*v.x() + v1.y()*v.y();

    res.angle = atan2(det, dot);
    if(res.angle < 0) res.angle += static_cast<float>(M_PI)*2; // unroll to 0-360
    return res;
}

float Dialog::angleBetween(QVector2D v1, QVector2D v2)
{
    float res;

    float det = v1.x()*v2.y() - v1.y()*v2.x();
    float dot = v1.x()*v2.x() + v1.y()*v2.y();

    res = atan2(det, dot);
    if(res < 0) res += static_cast<float>(M_PI)*2; // unroll to 0-360
    return res;
}

std::vector<GLfloat> Dialog::compileText(const QString text, const QVector2D pos)
{
    std::vector<GLfloat> res;
    float xOffs = 0.f;

    for(int i=0; i<text.size(); i++) {
        int chr = static_cast<int>(text[i].toLatin1());
        std::vector<GLfloat> partialRes = mSVG.glFloatArray(chr, pos + QVector2D(xOffs, 0));
        res.insert(res.end(), partialRes.begin(), partialRes.end());
        xOffs += mSVG.mPoints[chr].width();
    }

    return res;
}

float Dialog::displayText(QString text, QVector2D pos)
{
    float xOffs = 0.f;

    for(int i=0; i<text.size(); i++) {
        int chr = static_cast<int>(text[i].toLatin1());
        ui->mGLWidget->addToVBO(mSVG.glFloatArray(chr, pos + QVector2D(xOffs, 0)), GL_LINES);
        xOffs += mSVG.mPoints[chr].width();
    }

    return xOffs;
}

void Dialog::drawSplines()
{
    displayText("0,0", QVector2D(0,0));

    QVector2D prev;

    float alpha = static_cast<float>(ui->mAlpha->value());
    float tension = static_cast<float>(ui->mTension->value());
    float totalLen = mPath.calcTotalLen(static_cast<float>(ui->mAlpha->value()), static_cast<float>(ui->mTension->value()));
    float teethWidth = totalLen / ui->mTeeth->value();
    float samplingStep = teethWidth / ui->mSamples->value();

    mGear.clear();
    mSpline.clear();

    // the first point
    QVector2D fp = mPath.getSplinePoint(0.f, alpha, tension);
    mSpline.push_back(sPoint{fp, calcAngleForPoint(fp).angle});

    mGear.push_back(sPoint{fp, 0.f});

    for(float dist=0.f; dist<=totalLen; dist+=samplingStep) {
        float t = mPath.getT(dist, alpha, tension);

        if(t<0.f) {
            qDebug() << dist << "is over" << totalLen;
            continue;
        }

        float normalized = dist / teethWidth;
        if(normalized > 1.0f) normalized -= static_cast<int>(normalized);

        float sh = sample2(normalized);

        //qDebug() << "sampled height:" << sh;

        QVector2D p = mPath.getSplinePoint(t, alpha, tension);
        QVector2D s = mPath.getSplineGradient(t, alpha, tension);

        mSpline.push_back(sPoint{p, calcAngleForPoint(p).angle});

        float r = atan2(-s.y(), s.x());
        QVector2D rp(-(teethWidth/4)*sh*sin(r)+p.x(), -(teethWidth/4)*sh*cos(r)+p.y());

        if(dist > 0.f) {
            mGear.push_back(sPoint{rp, 0.f});
        }

        prev = rp;
    }

    if(ui->mGearGroupBox->isChecked())
        ui->mGLWidget->addToVBO(mGear.glFloatArray());

    ui->mGLWidget->addToVBO(mSpline.glFloatArray());
}

void Dialog::drawSystem()
{
    PolySegs system;
    for(int i=-2000; i<=2000; i+=100)
    {
        system.push_back(Seg2f(QVector2D(i,-2000), QVector2D(i, 2000)));
        system.push_back(Seg2f(QVector2D(-2000,i), QVector2D(2000,i)));
    }

    ui->mGLWidget->addToVBO(system.glFloatArray(-2.5f), GL_LINES, QVector4D(0.6, 0.2, 0.2, 1));
}

void Dialog::drawControlPoints()
{
    for(auto v: mPath.mPoints) {
        sPolygon p1, p2, p3;
        for(int i=0; i<=360; i+=10) {
            float phi = d2r(i);
            p1.push_back(QVector2D(sin(phi)*2, cos(phi)*2));
            p2.push_back(QVector2D(sin(phi)*2.5f, cos(phi)*2.5f));
            p3.push_back(QVector2D(sin(phi)*3, cos(phi)*3));
        }
        p1.translate(v);
        p2.translate(v);
        p3.translate(v);
        ui->mGLWidget->addToVBO(p1.glFloatArray(0.5f), GL_LINES, QVector4D(0,0,0,1));
        ui->mGLWidget->addToVBO(p2.glFloatArray(0.5f), GL_LINES, QVector4D(1.0f,0.8f,0,1));
        ui->mGLWidget->addToVBO(p3.glFloatArray(0.5f), GL_LINES, QVector4D(0,0,0,1));
    }
}

float Dialog::sample(float t)
{
    if(t<0.5) {
        return -(abs(t*5-1.25)-1.25);
    } else {
        return abs(t*5-3.75)-1.25;
    }
}

float Dialog::sample2(float t)
{
    if(t<0.5f) {
        t = t*4-1;
        return qPow(1.0-qPow(t,2.0),0.5);
    } else {
        t = t-0.5f;
        t = t*4-1;
        return -qPow(1.0-qPow(t,2.0),0.5);
    }
}

void Dialog::rotatePath(Path& p, float angle)
{
    for(size_t i=0; i<p.size(); i++) {
        float si = sin(angle);
        float co = cos(angle);

        float tx = p[i].X;
        float ty = p[i].Y;

        p[i].X = ((co * tx) - (si * ty));
        p[i].Y = ((si * tx) + (co * ty));
    }
}

void Dialog::translatePath(Path& p, float x)
{
    for(size_t i=0; i<p.size(); i++) {
        p[i].X += x;
    }
}

// cheating here!
float Dialog::iterate(float ccdist)
{
    float cntAngle1 = 0.f;
    float cntAngle2 = 0.f;

    //qDebug() << "iterate using ccdist:" << ccdist;

    for(size_t i=0; i<mSpline.size(); ++i) {
        size_t j = i+1; if(j==mSpline.size()) j=0;
        float r1 = mSpline.at(i).length();
        float r2 = ccdist - mSpline.at(i).length();
        float aDiff = angleBetween(mSpline.at(i), mSpline.at(j));
        float ratio = r1 / r2;
        float oAngle = aDiff * ratio;
        cntAngle1 += aDiff;
        cntAngle2 += oAngle;
    }

    return cntAngle2;
}

void Dialog::on_mCalculateButton_clicked()
{
    Clipper c;
    Paths solutions;

    mGear2.clear();

    ui->mEditModeGroupBox->setChecked(false);
    ui->mGLWidget->setFocus();

    float maxr = std::numeric_limits<float>::lowest();
    float rsum = 0.f;
    for(auto &p : mSpline) {
        if(p.length() > maxr) maxr = p.length();
        rsum += p.length();
    }

    float ccdist = (rsum / mSpline.size()) * 2;

    float res;
    do {
        res = r2d(iterate(ccdist));
        qd << res << res;
        ccdist -= pd(360.f, res);
    } while (fabs(360.0f - res) > 0.005f);

    // circle
    for(int i=0; i<=360; i+=10) {
        float phi = d2r(i);
        QVector2D v(sin(phi)*ccdist, cos(phi)*ccdist);
        mGear2 << IntPoint(v.x()*M, v.y()*M);
    }

    float alignmentAngle = angleBetween(mSpline.at(0), QVector2D(1,0));
    mSpline.rotate(alignmentAngle);
    mGear.rotate(alignmentAngle);

    Path gear = mGear.path();
    //writeDXF("c:/DRIVE/gear.dxf", gear);

    PolySegs cross1;
    float cs = ccdist / 10;
    cross1.push_back(Seg2f(QVector2D(0,-cs),QVector2D(0,cs)));
    cross1.push_back(Seg2f(QVector2D(-cs,0),QVector2D(cs,0)));

    PolySegs cross2(cross1);

    float textHeight = mSVG.mPoints[65].height();
    float textHeight3 = textHeight * 3;

    qDebug() << "spline size:" << mSpline.size();

    for(size_t i=0; i<mSpline.size(); ++i) {
        size_t j = i+1; if(j==mSpline.size()) j=0;

        float r1 = mSpline.at(i).length();
        float r2 = ccdist - mSpline.at(i).length();

        float aDiff = angleBetween(mSpline.at(i), mSpline.at(j));

        float ratio = r1 / r2;

        float oAngle = aDiff * ratio;

        mSpline.rotate(-aDiff);
        mGear.rotate(-aDiff);

        cross1.rotate(-aDiff);

        rotatePath(mGear2, oAngle);
        cross2.rotate(oAngle);

        if(ui->mGearGroupBox->isChecked())
            gear = mGear.path();
        else
            gear = mSpline.path();

        translatePath(gear, -ccdist*M);

        c.AddPath(gear, ptClip, true);
        c.AddPath(mGear2, ptSubject, true);
        c.Execute(ctDifference, solutions, pftNonZero, pftNonZero);
        c.Clear();

        // sort results baseed on the amount of points
        using tPair = std::pair<size_t,size_t>;
        std::vector<tPair> idxnsizes;
        for(size_t j=0; j<solutions.size(); j++)
            idxnsizes.push_back({j, solutions[j].size()});
        std::sort(idxnsizes.begin(), idxnsizes.end(), [](tPair a, tPair b){
            return a.second > b.second;
        });
        mGear2 = solutions[idxnsizes[0].first];

        ui->mGLWidget->clearVBOs();

        drawSystem();

        PolySegs vLine1, vLine2, vLine3;
        mGear.calcBounds();
        QVector2D ep1(-ccdist, -maxr-textHeight3);
        vLine1.push_back(Seg2f(QVector2D(-ccdist,0),ep1));

        // arrow1
        vLine1.push_back(Seg2f(ep1,ep1+QVector2D(textHeight,textHeight/3)));
        vLine1.push_back(Seg2f(ep1,ep1+QVector2D(textHeight,-textHeight/3)));

        QVector2D ep2(-ccdist + r1, -maxr-textHeight3);
        vLine2.push_back(Seg2f(QVector2D(-ccdist+r1,0),ep2));

        // arrow2
        vLine1.push_back(Seg2f(ep2,ep2+QVector2D(-textHeight,textHeight/3)));
        vLine1.push_back(Seg2f(ep2,ep2+QVector2D(-textHeight,-textHeight/3)));

        // arrow3
        vLine2.push_back(Seg2f(ep2,ep2+QVector2D(textHeight,textHeight/3)));
        vLine2.push_back(Seg2f(ep2,ep2+QVector2D(textHeight,-textHeight/3)));

        QVector2D ep3(0, -maxr-textHeight3);
        vLine3.push_back(Seg2f(QVector2D(0,0),ep3));

        // arrow3
        vLine3.push_back(Seg2f(ep3,ep3+QVector2D(-textHeight,textHeight/3)));
        vLine3.push_back(Seg2f(ep3,ep3+QVector2D(-textHeight,-textHeight/3)));

        // connect endpoints
        vLine1.push_back(Seg2f(ep1,ep3));

        displayText(ftoStr(r1), ep1+QVector2D(0,-textHeight));
        displayText(ftoStr(r2), ep2+QVector2D(0,-textHeight));

        ui->mGLWidget->addToVBO(vLine1.glFloatArray(), GL_LINES, QVector4D(0,1,1,1));
        ui->mGLWidget->addToVBO(vLine2.glFloatArray(), GL_LINES, QVector4D(0,1,1,1));
        ui->mGLWidget->addToVBO(vLine3.glFloatArray(), GL_LINES, QVector4D(0,1,1,1));

        ui->mGLWidget->addToVBO(cross1.translated(QVector2D(-ccdist,0)).glFloatArray(), GL_LINES, QVector4D(0,0,1,1));
        ui->mGLWidget->addToVBO(cross2.glFloatArray());

        ui->mGLWidget->addToVBO(mSpline.translated(QVector2D(-ccdist, 0)).glFloatArray(), GL_LINE_LOOP);

        ui->mGLWidget->addToVBO(pathToGLfloatArray(gear));
        ui->mGLWidget->addToVBO(pathToGLfloatArray(mGear2));

        //drawVectors(-ccdist, 0.f);

        //QPixmap pixmap = ui->widget->grab(QRect(0,0,ui->widget->width(), ui->widget->height()));
        //QString fname = QString("%1").arg(i, 3, 10, QChar('0'));
        //pixmap.save("C:/DRIVE/gear_frames/"+fname+".png");

        ui->mGLWidget->update();
        QApplication::processEvents();
    }

    //writeDXF("c:/DRIVE/gear2.dxf", path);
}

std::vector<GLfloat> Dialog::pathToGLfloatArray(Path& path)
{
    std::vector<GLfloat> res;
    for(size_t i=0; i<path.size(); i++) {
        size_t j = i+1; if(j==path.size()) j=0;
        res.push_back(path.at(i).X/M);
        res.push_back(path.at(i).Y/M);
        res.push_back(0.f);
        res.push_back(path.at(j).X/M);
        res.push_back(path.at(j).Y/M);
        res.push_back(0.f);
    }
    return res;
}

void Dialog::drawVectors(float x, float y)
{
    for(auto& v : mSpline){
        //todo for opengl
        //mScene->addLine(x, y, v.x()+x, v.y()+y);
    }
}

void Dialog::writeDXF(QString fname, Path poly)
{
    int id=115;
    writeFile(fname, readFile(":/dxf1.txt"));
    for(size_t i=0; i<poly.size(); i++) {
        size_t j=i+1; if(j==poly.size()) j=0;
        QString line = DXF_Line(id++, poly[i].X/M, poly[i].Y/M, 0, poly[j].X/M, poly[j].Y/M, 0);
        appendFile(fname, line.toUtf8());
    }
    appendFile(fname, readFile(":/dxf2.txt"));
}

QString Dialog::DXF_Line(int id, float x1, float y1, float z1, float x2, float y2, float z2)
{
    qd << x1 << y1 << x2 << y2;

    char b[4096]={0};
    int length;
    length = sprintf(b,
                     "LINE\r\n"
                     "5\r\n"
                     "%02X\r\n"
                     "330\r\n"
                     "1F\r\n"
                     "100\r\n"
                     "AcDbEntity\r\n"
                     "8\r\n"
                     "0\r\n"
                     "6\r\n"
                     "Continuous\r\n"
                     "62\r\n"
                     "7\r\n"
                     "100\r\n"
                     "AcDbLine\r\n"
                     "10\r\n"
                     "%f\r\n"
                     "20\r\n"
                     "%f\r\n"
                     "30\r\n"
                     "%f\r\n"
                     "11\r\n"
                     "%f\r\n"
                     "21\r\n"
                     "%f\r\n"
                     "31\r\n"
                     "%f\r\n"
                     "0\r\n",id, x1, y1, z1, x2, y2, z2);

    QString res(b);
    return res;
}

float Dialog::pd(float sp, float mv)
{
    float error = sp-mv;
    float derivative = error - mPrevError;
    float output = mP*error + mD*derivative;
    mPrevError = error;
    return output;
}

void Dialog::on_mAlpha_valueChanged(double /*arg1*/)
{
    drawScene();
}

void Dialog::on_mTension_valueChanged(double /*arg1*/)
{
    drawScene();
}

void Dialog::on_mGradientSlider_valueChanged(int /*value*/)
{
}

void Dialog::on_mTeeth_valueChanged(int /*arg1*/)
{
    drawScene();
}

void Dialog::on_mSamples_valueChanged(int /*arg1*/)
{
    drawScene();
}

void Dialog::on_mEditModeGroupBox_toggled(bool checked)
{
    if(checked) {
        drawScene();
    } else {
        ui->mGLWidget->destroyVBO("coords");
        ui->mGLWidget->destroyVBO("lineToCursor");
        ui->mGLWidget->destroyVBO("lineToClosest");
    }
}

void Dialog::on_mPerspectiveRadioButton_toggled(bool perspectiveProjection)
{
    ui->mGLWidget->updateProjection(perspectiveProjection);
}

void Dialog::on_mFrontButton_clicked()
{
    ui->mGLWidget->frontView();
}

void Dialog::on_mLeftButton_clicked()
{
    ui->mGLWidget->leftView();
}

void Dialog::on_mTopButton_clicked()
{
    ui->mGLWidget->topView();
}

void Dialog::on_mRightButton_clicked()
{
    ui->mGLWidget->rightView();
}

void Dialog::on_mGearGroupBox_toggled(bool gear)
{
    if(gear) {
        ui->mGearParamsFrame->show();
        ui->mFrictionDiscLabel->hide();
    } else {
        ui->mFrictionDiscLabel->show();
        ui->mGearParamsFrame->hide();
    }
    drawScene();
}

void Dialog::on_mSaveButton_clicked()
{
    if(mPath.mPoints.size() > 2) {
        QString fname;
        if(ui->mGearGroupBox->isChecked()) {
            fname = QFileDialog::getSaveFileName(this, tr("Save main gear"), "./gear1.dxf", tr("DXF file (*.dxf)"));
            writeDXF(fname, mGear.path());
        } else {
            fname = QFileDialog::getSaveFileName(this, tr("Save main friction disk"), "./friction_disc1.dxf", tr("DXF file (*.dxf)"));
            writeDXF(fname, mSpline.path());
        }
    }

    if(mGear2.size()) {
        QString fname;
        if(ui->mGearGroupBox->isChecked()) {
            fname = QFileDialog::getSaveFileName(this, tr("Save secondary gear"), "./gear2.dxf", tr("DXF file (*.dxf)"));
        } else {
            fname = QFileDialog::getSaveFileName(this, tr("Save main friction disk"), "./friction_disc1.dxf", tr("DXF file (*.dxf)"));
        }
        writeDXF(fname, mGear2);
    }

    QMessageBox::information(this, "Success", "Gear(s) saved successfully!");
}
