#include "switchbutton.h"
#include "qpainter.h"
#include "qevent.h"
#include "qtimer.h"
#include "qdebug.h"

SwitchButton::SwitchButton(QWidget *parent): QWidget(parent)
{
    checked = false;
    buttonStyle = ButtonStyle_Rect;

    bgColorOff = QColor(225, 225, 225);
    bgColorOn = QColor(250, 250, 250);

    sliderColorOff = QColor(100, 100, 100);
    sliderColorOn = QColor(100, 184, 255);

    textColorOff = QColor(255, 255, 255);
    textColorOn = QColor(10, 10, 10);

    textOff = "";
    textOn = "";

    imageOff = ":/image/btncheckoff1.png";
    imageOn = ":/image/btncheckon1.png";

    space = 2;
    rectRadius = 5;

    step = width() / 20;
    startX = 0;
    endX = 0;

    timer = new QTimer(this);
    timer->setInterval(5);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateValue()));

    setFont(QFont("Microsoft Yahei", 10));
}

SwitchButton::~SwitchButton()
{

}

void SwitchButton::mousePressEvent(QMouseEvent *)
{
    checked = !checked;
    emit checkedChanged(checked);

    if (checked) {
        if (buttonStyle == ButtonStyle_Rect) {
            endX = width() - width() / 2;
        } else if (buttonStyle == ButtonStyle_CircleIn) {
            endX = width() - height();
        } else if (buttonStyle == ButtonStyle_CircleOut) {
            endX = width() - height() + space;
        }
    } else {
        endX = 0;
    }

    timer->start();
}

void SwitchButton::resizeEvent(QResizeEvent *)
{
    step = width() / 20;

    if (checked) {
        if (buttonStyle == ButtonStyle_Rect) {
            startX = width() - width() / 2;
        } else if (buttonStyle == ButtonStyle_CircleIn) {
            startX = width() - height();
        } else if (buttonStyle == ButtonStyle_CircleOut) {
            startX = width() - height() + space;
        }
    } else {
        startX = 0;
    }

    update();
}

void SwitchButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (buttonStyle == ButtonStyle_Image) {
        drawImage(&painter);
    } else {
        drawBg(&painter);
        drawSlider(&painter);
        drawText(&painter);
    }
}

void SwitchButton::drawBg(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    if (!checked) {
        painter->setBrush(bgColorOff);
    } else {
        painter->setBrush(bgColorOn);
    }

    if (buttonStyle == ButtonStyle_Rect) {
        painter->drawRoundedRect(rect(), rectRadius, rectRadius);
    } else if (buttonStyle == ButtonStyle_CircleIn) {
        QRect rect(0, 0, width(), height());

        int radius = rect.height() / 2;

        int circleWidth = rect.height();

        QPainterPath path;
        path.moveTo(radius, rect.left());
        path.arcTo(QRectF(rect.left(), rect.top(), circleWidth, circleWidth), 90, 180);
        path.lineTo(rect.width() - radius, rect.height());
        path.arcTo(QRectF(rect.width() - rect.height(), rect.top(), circleWidth, circleWidth), 270, 180);
        path.lineTo(radius, rect.top());

        painter->drawPath(path);
    } else if (buttonStyle == ButtonStyle_CircleOut) {
        QRect rect(space, space, width() - space * 2, height() - space * 2);
        painter->drawRoundedRect(rect, rectRadius, rectRadius);
    }

    painter->restore();
}

void SwitchButton::drawSlider(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    if (!checked) {
        painter->setBrush(sliderColorOff);
    } else {
        painter->setBrush(sliderColorOn);
    }

    if (buttonStyle == ButtonStyle_Rect) {
        int sliderWidth = width() / 2 - space * 2;
        int sliderHeight = height() - space * 2;
        QRect sliderRect(startX + space, space, sliderWidth , sliderHeight);
        painter->drawRoundedRect(sliderRect, rectRadius, rectRadius);
    } else if (buttonStyle == ButtonStyle_CircleIn) {
        QRect rect(0, 0, width(), height());
        int sliderWidth = rect.height() - space * 2;
        QRect sliderRect(startX + space, space, sliderWidth, sliderWidth);
        painter->drawEllipse(sliderRect);
    } else if (buttonStyle == ButtonStyle_CircleOut) {
        QRect rect(0, 0, width() - space, height() - space);
        int sliderWidth = rect.height();
        QRect sliderRect(startX, space / 2, sliderWidth, sliderWidth);
        painter->drawEllipse(sliderRect);
    }

    painter->restore();
}

void SwitchButton::drawText(QPainter *painter)
{
    painter->save();

    if (!checked) {
        painter->setPen(textColorOff);
        painter->drawText(width() / 2, 0, width() / 2 - space, height(), Qt::AlignCenter, textOff);
    } else {
        painter->setPen(textColorOn);
        painter->drawText(0, 0, width() / 2 + space * 2, height(), Qt::AlignCenter, textOn);
    }

    painter->restore();
}

void SwitchButton::drawImage(QPainter *painter)
{
    painter->save();

    QPixmap pix;

    if (!checked) {
        pix = QPixmap(imageOff);
    } else {
        pix = QPixmap(imageOn);
    }

    int targetWidth = pix.width();
    int targetHeight = pix.height();
    pix = pix.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int pixX = rect().center().x() - targetWidth / 2;
    int pixY = rect().center().y() - targetHeight / 2;
    QPoint point(pixX, pixY);
    painter->drawPixmap(point, pix);

    painter->restore();
}

void SwitchButton::updateValue()
{
    if (checked) {
        if (startX < endX) {
            startX = startX + step;
        } else {
            startX = endX;
            timer->stop();
        }
    } else {
        if (startX > endX) {
            startX = startX - step;
        } else {
            startX = endX;
            timer->stop();
        }
    }

    update();
}

void SwitchButton::setChecked(bool checked)
{
    if (this->checked != checked) {
        this->checked = checked;

        if (checked) {
            if (buttonStyle == ButtonStyle_Rect) {
                endX = width() - width() / 2;
            } else if (buttonStyle == ButtonStyle_CircleIn) {
                endX = width() - height();
            } else if (buttonStyle == ButtonStyle_CircleOut) {
                endX = width() - height() + space;
            }
        } else {
            endX = 0;
        }
        timer->start();
        emit checkedChanged(checked);
    }
}

void SwitchButton::setButtonStyle(SwitchButton::ButtonStyle buttonStyle)
{
    this->buttonStyle = buttonStyle;
    update();
}

void SwitchButton::setBgColor(QColor bgColorOff, QColor bgColorOn)
{
    this->bgColorOff = bgColorOff;
    this->bgColorOn = bgColorOn;
    update();
}

void SwitchButton::setSliderColor(QColor sliderColorOff, QColor sliderColorOn)
{
    this->sliderColorOff = sliderColorOff;
    this->sliderColorOn = sliderColorOn;
    update();
}

void SwitchButton::setTextColor(QColor textColorOff, QColor textColorOn)
{
    this->textColorOff = textColorOff;
    this->textColorOn = textColorOn;
    update();
}

void SwitchButton::setText(QString textOff, QString textOn)
{
    this->textOff = textOff;
    this->textOn = textOn;
    update();
}

void SwitchButton::setImage(QString imageOff, QString imageOn)
{
    this->imageOff = imageOff;
    this->imageOn = imageOn;
    update();
}

void SwitchButton::setSpace(int space)
{
    this->space = space;
    update();
}

void SwitchButton::setRectRadius(int rectRadius)
{
    this->rectRadius = rectRadius;
    update();
}
