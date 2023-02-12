#pragma once
#include <vector>
#include <qpoint.h>
#include <qcolor.h>
#include <qsize.h>
#include "class_declarations.h"
struct Cell
{
    //QPoint point;
    QColor color;
    std::pair<int, int> position;

    //static Cell2D& update_pts(Cell2D&);
    Cell() {};
    Cell(QColor, std::pair<int, int>);
    Cell(QColor, int, int);
};

bool compare(QSize, QSize);