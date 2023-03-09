#include "UserInterface.h"

UserInterface::UserInterface() :
    art_station(PixelArt(InitArtDialog::initialise(), this)),
    screen_dim(screen_rect.width(), screen_rect.height()),
    ui_painter()
{
    setWindowTitle("PixelArt");
    setObjectName("Ui");
    art_station.setObjectName("Viewport");

    QPalette pal;
    pal.setColor(QPalette::Base, QColor(255, 202, 161));
    pal.setColor(QPalette::AlternateBase, QColor(163, 227, 255));
    setPalette(pal);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setScreen();
    showMaximized();  
    art_station.show();
    art_station.setFocus();
}

void UserInterface::setScreen()
{
    const QScreen* at_screen = nullptr;
    int s_size = 0;

    for (auto& screen : QApplication::screens())
    {
        int cmpr_size = screen->size().width() * screen->size().height();
        if (s_size <= cmpr_size) { s_size = cmpr_size; at_screen = screen; }
    }
    if (at_screen) setGeometry(at_screen->geometry());
}

void UserInterface::resizeEvent(QResizeEvent* event)
{
    art_station.setGeometry
    (
        100, 50,
        geometry().width() - 600, geometry().height() - 100
    );
    art_station.setCanvas_pos();
}
void UserInterface::paintEvent(QPaintEvent*)
{
    ui_painter.begin(this);
    ui_painter.setPen(QPen(Qt::darkGray, 3));
    ui_painter.drawRect(art_station.geometry().adjusted(-2, -2, 1, 1));
    ui_painter.end();
}

