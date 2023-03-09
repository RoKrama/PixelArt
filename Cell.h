#pragma once
#include <vector>
#include <qpoint.h>
#include <qcolor.h>
#include <qsize.h>
#include "class_declarations.h"
struct Cell
{
    QColor color;
    std::pair<int, int> position;

    Cell();
    Cell(QColor, std::pair<int, int>);
    Cell(QColor, int, int);

    const bool operator ==(const Cell&) const;
    const bool operator !=(const Cell&) const;
};