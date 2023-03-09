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
#include <mutex>
#include "Cell.h"
#include <vector>
class PixelArt : public QWidget
{
    Q_OBJECT;

    PixelArt(const PixelArt&) = delete;
    PixelArt& operator=(const PixelArt&) = delete;
    // deleted cpy_ctor and assigment to prevent making copies
    PixelArt() = delete;

    const int x_n, y_n;
    int cell_size;
    QSize q_cell_size;
    QRect img_rect, intersected;
    std::pair<QPixmap, QPixmap> img_pair;
    QPixmap background;
    QPixmap* canvas;
    std::vector<unsigned int> fib;
    int zoom;
    Cell2D cell_map;
    QPoint canvas_pos, back_pos, move_pos, canvas_offset;

    QPainter painter, line_painter;
    QColor current_color;
    QColorDialog color_dialog;

    QList<QLine> line_list_x, line_list_y;
    QList<QLine> thick_lines_list;
    QPen line_pen;

    const QShortcut undo, redo, move_canvas, open_color_dialog, show_thick_lines;
    bool moving_canvas, clicked_in_canvas, clipped, thick_lines;

    using Cache = std::list<Cell>;
    Cache undo_cache, redo_cache;


    inline void constructLines();
    inline void constructCanvas();
    inline void constructShortcuts();

    void mousePressEvent(QMouseEvent*) override final;
    void mouseMoveEvent(QMouseEvent*) override final;
    void mouseReleaseEvent(QMouseEvent*) override final;
    void wheelEvent(QWheelEvent*) override final;
    void keyReleaseEvent(QKeyEvent*) override final;
    void paintEvent(QPaintEvent*) override final;

    std::pair<int, int> quantise_m_pos(QPoint);
    QColor draw_rect(const std::pair<int, int>&, const QColor);
    bool cell_same(const std::pair<int, int>&);
    inline void update_cell_map(std::pair<int, int>, QColor);

    inline void zoomClipFn(int, const QPoint&);
    void draw_in_range(const QRect&);
    QSize clipped_canvas_size();
    inline void set_clipped_canvas_pos();
    inline void set_offset();
    void paintLines();
    void set_bg_clipped_pos();

    void undo_fn(bool);

public:
    void setCanvas_pos();
    PixelArt(const InitReturn, QWidget*);
    ~PixelArt();
};
