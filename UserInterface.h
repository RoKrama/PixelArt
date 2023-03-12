#pragma once
#include "PixelArt.h"
#include "InitArt.h"
#include <QResizeEvent>
#include <qpainterpath.h>

class UserInterface : public QWidget
{
    Viewport art_station;

    void setScreen();
    QRect screen_rect;
    QSize screen_dim;


    QPainter ui_painter;

    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent*) override;

public:
    UserInterface();
    ~UserInterface() {};
};

