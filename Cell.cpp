#include "Cell.h"

//Cell2D& Cell::update_pts(Cell2D& map)
//{
//	//for(auto &ref_first : map)
//	//	for(auto &ref_second : ref_first)
//	//		ref_second.point()
//}

Cell::Cell() : color(), position() {}

Cell::Cell(QColor c, std::pair<int, int> pos):
	color(c), position(pos)
{}

Cell::Cell(QColor c, int x, int y):
	color(c), position(std::pair<int,int>(x,y))
{}

bool compare(QSize first, QSize second )
{
	return (
		first.width()  * first.height() >
		second.width() * second.height()
	);
}