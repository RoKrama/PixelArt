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


    QColorDialog color_dialog;
    inline void constructCanvas();
    inline void constructShortcuts();

    const int x_n, y_n;
    const QSize cell_size;
    QImage canvas, background;
    //float fps= 0;
    QPoint canvas_pos, move_pos;
    QList<QLine> line_list;
    void paintLines();
    QPainter painter;
    QPen line_pen;

    bool quantise_m_pos(QPoint&);

    QColor current_clr;
    QColor draw_rect(const QPoint, const QColor);

    const QShortcut undo, redo, move_canvas, open_color_dialog;
    bool moving_canvas, clicked_in_canvas;
    void undo_fn(bool);
    pix::Cache undo_cache, redo_cache;

    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;

public:
    void setCanvas_pos();
    PixelArt(const pix::InitReturn, QWidget*);
    ~PixelArt();
};
