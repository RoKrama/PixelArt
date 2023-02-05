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
#include <Windows.h>
#include <thread>
#include "Cell.h"
#include <vector>

class PixelArt : public QWidget
{
    Q_OBJECT

    PixelArt(const PixelArt&) = delete;
    PixelArt& operator=(const PixelArt) = delete;
    // deleted cpy_ctor and assigment to prevent making copies

    const int x_n, y_n;
    int cell_size;

    QImage canvas, background,
        sc_canvas, sc_background;
    float calc1 = 0;
    Cell2D cell_map;
    QPoint canvas_pos, move_pos;

    QPainter painter;
    QColor current_color;
    QColorDialog color_dialog;

    QList<QLine> line_list;
    QList<QLine> thick_lines_list;
    QPen line_pen;

    const QShortcut undo, redo, move_canvas, open_color_dialog,
        show_thick_lines;
    bool moving_canvas, clicked_in_canvas, scale_clipping, 
        thick_lines;
    using Cache = std::deque<Cell>;
    Cache undo_cache, redo_cache;

    void paintLines();
    void quantise_m_pos(QPoint&);
    QColor draw_rect(const QPoint, const QColor);
    void zoomFn(int);
    void zoomClipFn(int);
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
    PixelArt(const InitReturn, QWidget*);
    ~PixelArt();
};
