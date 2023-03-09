#include "Cell.h"
Cell::Cell() : color(), position() {}

Cell::Cell(QColor c, std::pair<int, int> pos):
	color(c), position(pos)
{}

Cell::Cell(QColor c, int x, int y):
	color(c), position(std::pair<int,int>(x,y))
{}

const bool Cell::operator == (const Cell& other) const
{
	return (other.position == position);
}
const bool Cell::operator != (const Cell& other) const
{
	return (other.position != position);
}