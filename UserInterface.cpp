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
    art_station.setCanvas_pos();
    art_station.show();
   
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
        100, 50,
        geometry().width() - 600, geometry().height() - 100
    );
    art_station.setCanvas_pos();
}
void UserInterface::paintEvent(QPaintEvent*)
{
    painter.begin(this);
    QPen pen(Qt::darkGray, 3);
    pen.setCapStyle(Qt::SquareCap);
    painter.setPen(pen);
    painter.drawRect(art_station.geometry().adjusted(-3, -2, 1, 1));
    painter.end();
}
