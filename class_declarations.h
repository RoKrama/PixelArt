#pragma once
#include <vector>
class PixelArt;
class InitArt;
class InitArtDialog;
class Cell;
class UserInterface;

using  Cell2D = std::vector < std::vector < Cell>>;

struct InitReturn
{
	int cells_x, cells_y, cell_size;
	InitReturn() :
		cells_x(0), cells_y(0), cell_size(0) {};
	InitReturn(int x, int y, int s) :
		cells_x(x), cells_y(y), cell_size(s) {};
};

namespace pix
{
	static inline bool& bool_switch(bool& b) 
	{
		if (b) return (b = false);
		else return (b = true);
	}

	// returns val % mod if not 0 else returns mod
	inline int full_mod(int val, int mod)
	{
		if ((val %= mod) == 0) return mod;
		else return val;
	}
}

