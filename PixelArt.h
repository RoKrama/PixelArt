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
    PixelArt& operator=(const PixelArt&) = delete;
    // deleted cpy_ctor and assigment to prevent making copies

    const int x_n, y_n;
    int cell_size;

    QRect img_rect, intersected, aspect;
    using Draw_range = std::pair<std::pair<int, int>, std::pair<int, int>>;
    QImage canvas, background;
    Cell2D cell_map;
    QPoint canvas_pos, back_pos, move_pos, canvas_offset;

    QPainter painter, line_painter;
    QColor current_color;
    QColorDialog color_dialog;

    QList<QLine> line_list_x, line_list_y;
    QList<QLine> thick_lines_list;
    QPen line_pen;

    const QShortcut undo, redo, move_canvas, open_color_dialog,
        show_thick_lines;
    bool moving_canvas, clicked_in_canvas, scale_clipping,
        thick_lines;
    using Cache = std::deque<Cell>;
    Cache undo_cache, redo_cache;

    void range(int* const);
    int  range(int);
    int irange(int);
    void irange(int* const);

    void paintLines();
    void paintUpdatedLines();
    std::pair<int, int> quantise_m_pos(QPoint);
    QColor draw_rect(const std::pair<int, int>&, const QColor);
    void update_cell_map(std::pair<int, int>, QColor);
    Draw_range set_draw_range(const QRect&, const QRect&);
    void draw_in_range(const Draw_range&);
    void update_lines();
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
