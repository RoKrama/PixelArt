#include "UserInterface.h"

UserInterface::UserInterface() :
    art_station(PixelArt(InitArtDialog::initialise(), this)),

    screen_rect(setScreen()),
    screen_dim(screen_rect.width(), screen_rect.height()),

    painter()
{

    setWindowTitle("PixelArt");
    setScreen();
    setPalette(QPalette(Qt::black));
    showMaximized();
    art_station.show();
    art_station.setCanvas_pos(); 
}

QRect UserInterface::setScreen()
{
    const QScreen* at_screen = nullptr;
    int s_size = 0;
    QRect screen_geom;

    for (auto& screen : QApplication::screens())
    {
        int cmpr_size = screen->size().width() * screen->size().height();
        if (s_size <= cmpr_size) { s_size = cmpr_size; at_screen = screen; }
    }
    if (at_screen) setGeometry(screen_geom = at_screen->geometry());
    return screen_geom;
}

void UserInterface::resizeEvent(QResizeEvent* event)
{
    art_station.setGeometry
    (
        200, 100,
        geometry().width() - 400, geometry().height() - 200
    );
    art_station.setCanvas_pos();
}
void UserInterface::paintEvent(QPaintEvent*)
{
    painter.begin(this);
    QPen pen(Qt::darkBlue, 3);
    pen.setCapStyle(Qt::SquareCap);
    painter.setPen(pen);
    painter.drawRect(art_station.geometry().adjusted(-3, -2, 1, 1));
    painter.end();
}
