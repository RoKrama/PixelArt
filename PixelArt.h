#pragma once

#include "class_declarations.h"
#include <QtWidgets/QWidget>
#include <deque>
#include <chrono>
#include <qimage.h>
#include <QPushButton>
#include <qevent.h>
#include <qpainter.h>
#include <qshortcut.h>
#include <qapplication.h>
#include <qlist.h>
#include <qcolordialog.h>
#include <iostream>
namespace pix
{
    using Cell = std::pair<QPoint, QColor>;
    using Cache = std::deque<Cell>;
}
class PixelArt : public QWidget
{
    Q_OBJECT

    PixelArt(const PixelArt&) = delete;
    PixelArt& operator=(const PixelArt) = delete;
    // deleted cpy_ctor and assigment to prevent making copies

    const int x_n, y_n;
    int cell_size;

    QImage canvas, background;
    QPoint canvas_pos, move_pos;

    QPainter painter;
    QColor current_color;
    QColorDialog color_dialog;

    QList<QLine> line_list;
    QPen line_pen;

    const QShortcut undo, redo, move_canvas, open_color_dialog;
    bool moving_canvas, clicked_in_canvas;
    pix::Cache undo_cache, redo_cache;

    void paintLines();
    void quantise_m_pos(QPoint&);
    QColor draw_rect(const QPoint, const QColor);
    void zoomFn(int);
    void undo_fn(bool);

    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;

    inline void constructLines();;
    inline void constructCanvas();
    inline void constructShortcuts();

public:
    void setCanvas_pos();
    PixelArt(const pix::InitReturn, QWidget*);
    ~PixelArt();
};
