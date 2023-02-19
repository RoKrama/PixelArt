#include "PixelArt.h"
PixelArt::PixelArt(const InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size), // cell vals.cell_size in pix
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells

	canvas(),
	img_rect(QPoint(0, 0), QSize(x_n* cell_size, y_n* cell_size)),
	intersected(img_rect),
	line_list_x(), line_list_y(),
	canvas_pos(0, 0), background(canvas),
	back_pos(0, 0),
	canvas_offset(0, 0),
	painter(), line_pen(Qt::gray, 1),

	undo(QKeySequence::Undo, this),
	redo(QKeySequence::Redo, this),
	move_canvas(QKeySequence(Qt::CTRL | Qt::Key_M), this),
	open_color_dialog(QKeySequence(Qt::CTRL | Qt::Key_C), this),
	show_thick_lines(QKeySequence(Qt::CTRL | Qt::Key_T), this),

	moving_canvas(false), clicked_in_canvas(false),
	scale_clipping(false), thick_lines(false),
	undo_cache(), redo_cache(),

	color_dialog(QColorDialog(this))
{
	for (int i = 0; i < x_n; i++)
	{
		cell_map.push_back(std::vector<Cell>());
		for (int j = 0; j < y_n; j++)
			cell_map[i].push_back(Cell(QColor(std::abs(i-255) % 255, j % 255, (i * j) % 255), i, j));
	}
	line_list_x.reserve(x_n + 1);
	line_list_y.reserve(y_n + 1);
	// moving display rect for canvas position
	img_rect.moveTopLeft(canvas_pos);

	constructCanvas();
	constructShortcuts();
	setFocusPolicy(Qt::StrongFocus);
	connect(
		&color_dialog, &QColorDialog::colorSelected,
		this, [this] { current_color = color_dialog.selectedColor(); }
	);
}
// CONSTRUCTOR FUNCTIONS
inline void PixelArt::constructCanvas()
{
	constructLines();
	canvas.fill(Qt::white);
	paintLines();
}
inline void PixelArt::constructShortcuts()
{
	connect( // CTRL_Z
		&undo, &QShortcut::activated,
		this, [this] { undo_fn(true); }
	);
	connect( // CTRL_Y
		&redo, &QShortcut::activated,
		this, [this] { undo_fn(false); }
	);
	connect( // CTRL_M
		&move_canvas, &QShortcut::activated,
		this, [this] { moving_canvas = true; }
	);
	connect( // CTRL_C
		&open_color_dialog, &QShortcut::activated,
		this, [this] { color_dialog.show(); }
	);
	connect( // CTRL_T
		&show_thick_lines, &QShortcut::activated,
		this, [this]
		{
			thick_lines ? thick_lines = false
				: thick_lines = true;
			update();
		}
	);
}
inline void PixelArt::constructLines()
{
	QSize background_size = background.size();
	// pixel lines
	std::thread t1(
		[this, &background_size]() mutable 
		{
			for (int i = 0; i <= background_size.width() / cell_size; i++)
				line_list_x.push_back(
					QLine(
						i * cell_size,
						0,
						i * cell_size,
						background_size.height() + cell_size
					)
				);
		}
	);
	for (int i = 0; i <= background_size.height() / cell_size; i++)
		line_list_y.push_back(
			QLine(
				0,
				i * cell_size,
				background_size.width() + cell_size,
				i * cell_size
			)
		);

	// orientation grid
	if (!thick_lines_list.empty()) thick_lines_list.clear();
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				i * ((x_n * cell_size) / 3),
				0,
				i * ((x_n * cell_size) / 3),
				img_rect.height() + cell_size
			)
		);
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				0,
				i * ((y_n * cell_size) / 3),
				img_rect.width() + cell_size,
				i * ((y_n * cell_size) / 3)
			)
		);
	t1.join();
}

// CLASS FUNCTIONS
void PixelArt::update_lines()
{
	Draw_range d_range = set_draw_range(intersected, rect());
	QList<QLine>::iterator iter_x = line_list_x.begin();
	QList<QLine>::iterator iter_y = line_list_y.begin();
	QList<QLine>::iterator iter_x_end =
		iter_x + d_range.first.second - d_range.first.first;
	QList<QLine>::iterator iter_y_end =
		iter_y + d_range.second.second - d_range.second.first;
	iter_x += d_range.first.first;
	iter_y += d_range.second.first;
	while (iter_x != iter_x_end)
	{
		iter_x->p1().setX(intersected.left());
		iter_x->p2().setX(intersected.left() + intersected.width());
	}
	while (iter_y != iter_y_end)
	{
		iter_y->p1().setY(intersected.top());
		iter_y->p2().setY(intersected.top() + intersected.height());
	}
}
QColor PixelArt::draw_rect(const std::pair<int, int>& at, const QColor with)
{
	QPoint position(
		at.first * cell_size + img_rect.left(),
		at.second * cell_size + img_rect.top()
	);
	QRect area(position, QSize(cell_size, cell_size));

	painter.begin(&canvas);
	if (with == Qt::transparent)
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(area, with);
	painter.end();

	// clip for faster load
	update(area);

	return with;
}
std::pair<int, int> PixelArt::quantise_m_pos(QPoint mouse_pos)
{
	// get point relative to canvas position
	mouse_pos -= img_rect.topLeft();

	return std::pair<int, int>(
		mouse_pos.x() / cell_size,
		mouse_pos.y() / cell_size
		);
}
void PixelArt::setCanvas_pos()
{
	canvas = QImage(rect().size(), QImage::Format::Format_ARGB32);
	img_rect.moveTopLeft(
		rect().center() + (img_rect.topLeft() - img_rect.center())
	);
	intersected = img_rect.intersected(rect());
	back_pos = intersected.topLeft() + canvas_offset;
	draw_in_range(set_draw_range(img_rect, rect()));

	background = QImage(
		intersected.size() - QSize(canvas_offset.x(), canvas_offset.y()),
		QImage::Format::Format_ARGB32
	);
	paintLines();
	update();
}
void PixelArt::paintLines()
{
	constructLines();
	background.fill(Qt::transparent);
	line_painter.begin(&background);
	line_painter.setPen(line_pen);
	line_painter.drawLines(line_list_x);
	line_painter.drawLines(line_list_y);
	line_painter.end();
}
void PixelArt::paintUpdatedLines()
{
	Draw_range d_range = set_draw_range(img_rect, rect());

	background.fill(Qt::transparent);
	line_painter.begin(&background);
	line_painter.setPen(line_pen);
	for (int i = 0; i < (d_range.second.second - d_range.second.first); i++)
		line_painter.drawLine(
			QLine(
				0,
				i * cell_size,
				intersected.width(),
				i * cell_size
			)
		);
	for (int i = 0; i < (d_range.first.second - d_range.first.first); i++)
		line_painter.drawLine(
			QLine(
				i * cell_size,
				0,
				i * cell_size,
				intersected.height()
			)
		);
	line_painter.end();
}
void PixelArt::update_cell_map(std::pair<int, int> pos, QColor color)
{
	cell_map[pos.first][pos.second].color = color;
}
PixelArt::Draw_range PixelArt::set_draw_range(const QRect& clip, const QRect& show)
{
	int dx_r = ((img_rect.right() - show.right()) * x_n) / img_rect.width();
	int dy_r = ((img_rect.bottom() - show.bottom()) * y_n) / img_rect.height();
	int dx   = (std::abs(intersected.left() - clip.left()) * x_n) / img_rect.width();
	int dy   = (std::abs(intersected.top() - clip.top()) * y_n) / img_rect.height();

	range(&dx_r); range(&dy_r);

	return
		Draw_range(
			std::pair<int, int>(dx, x_n - dx_r),
			std::pair<int, int>(dy, y_n - dy_r)
		);
}
void PixelArt::draw_in_range(const Draw_range& d_range)
{
	painter.begin(&canvas);
	for (int i = d_range.first.first; i < d_range.first.second; i++)
		for (int j = d_range.second.first; j < d_range.second.second; j++)
			painter.fillRect(
				QRect(
					QPoint(i * cell_size, j * cell_size) + img_rect.topLeft(),
					QSize(cell_size, cell_size)
				),
				cell_map[i][j].color
			);
	painter.end();
}
// EVENTS 
void PixelArt::mousePressEvent(QMouseEvent* event)
{
	if (intersected.contains(event->pos()))
	{
		if (moving_canvas) // ctrl + M
		{
			clicked_in_canvas = true; // allow moving
			move_pos = event->pos(); // record mouse_pos in class member
			return;	// moving canvas will be handled in mouseMoveEvent
		}
		if (
			event->button() == Qt::MouseButton::LeftButton &&
			!(GetAsyncKeyState(0x11) < 0)
			)
		{
			// getting clicked cell position in int, int
			std::pair<int, int> p_index(quantise_m_pos(event->pos()));

			undo_cache.push_back( // before drawing over
				Cell( // constructing cell and loading into undo cache
					cell_map[p_index.first][p_index.second].color,
					p_index
				)
			);
			draw_rect(p_index, current_color);
			update_cell_map(p_index, current_color);
		}
	}
}
void PixelArt::mouseMoveEvent(QMouseEvent* event)
{
	auto t1 = std::chrono::high_resolution_clock::now();

	// moving of canvas happens if true
	if (!(moving_canvas && clicked_in_canvas)) return;

	QPoint move = (event->pos() - move_pos);
	QRect save(intersected.translated(move));
	QImage copy(canvas.copy(intersected));

	bool x_min = img_rect.left() < 0 && move.x() > 0;
	bool x_max = img_rect.left() + img_rect.width() > rect().width() && move.x() < 0;
	bool y_min = img_rect.top() < 0 && move.y() > 0;
	bool y_max = img_rect.top() + img_rect.height() > rect().height() && move.y() < 0;

	img_rect.translate(move);

	painter.begin(&canvas);
	painter.fillRect(intersected, Qt::black);
	painter.drawImage(intersected.topLeft() + move, copy);
	painter.end();

	intersected = img_rect.intersected(rect());
	canvas_offset.setX(range(img_rect.left() % cell_size));
	canvas_offset.setY(range(img_rect.top() % cell_size));

	if (x_min)
		draw_in_range(set_draw_range(
			img_rect,
			QRect(intersected.topLeft(), save.bottomLeft())
		));
	if (y_min)
		draw_in_range(set_draw_range(
			img_rect,
			QRect(intersected.topLeft(), save.topRight())
		));
	if (x_max)
		draw_in_range(set_draw_range(
			QRect(
				QPoint(save.right(), img_rect.top()),
				intersected.bottomRight()
			).translated(intersected.left() - img_rect.left(), 0),
			intersected
		));
	if (y_max)
		draw_in_range(set_draw_range(
			QRect(
				save.bottomLeft(), intersected.bottomRight()
			).translated(0, intersected.top() - img_rect.top()),
			intersected
		));

	back_pos = intersected.topLeft();
	canvas_pos += move;
	move_pos = event->pos();

	update();

	auto t2 = std::chrono::high_resolution_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()<< std::endl;
}
void PixelArt::mouseReleaseEvent(QMouseEvent* event)
{
	moving_canvas = false;
	clicked_in_canvas = false; // reset
}
void PixelArt::keyReleaseEvent(QKeyEvent* event)
{
	if (moving_canvas && event->key() == Qt::Key_M)
		moving_canvas = false; // reset
}
void PixelArt::wheelEvent(QWheelEvent* event)
{
	if (
		GetAsyncKeyState(0x11) < 0 &&
		intersected.contains(mapFromGlobal(QCursor::pos()))
	) // ctrl is held and mouse is in precondition
	{
		int direction = 0;
		event->angleDelta().y() > 0 ?
			direction = +1 : direction = -1;

		if (cell_size + direction > 1)
		{
			zoomClipFn(direction);
			update();
		}
		event->accept();
	}
	else event->ignore();
}
void PixelArt::zoomClipFn(int dir)
{
	cell_size += dir;

	img_rect.adjust(0, 0, dir * x_n, dir * y_n);

	// making zoom appear happening from center of the canvas
	img_rect.translate(-((dir * x_n) / 2), -((dir * y_n) / 2));

	painter.begin(&canvas);
	painter.fillRect(intersected, Qt::black);
	painter.end();

	// getting part of rect that should be shown
	intersected = img_rect.intersected(rect());

	canvas_offset.setX(range(img_rect.left() % cell_size));
	canvas_offset.setY(range(img_rect.top() % cell_size));

	std::thread th1( // background and lines painting thread
		[this]
		{
			background = QImage(
				intersected.size() - QSize(canvas_offset.x(), canvas_offset.y()),
				QImage::Format::Format_ARGB32
			);
			back_pos = img_rect.topLeft();
			paintUpdatedLines();
		}
		);
	//canvas_pos = intersected.topLeft();

	// getting data range and drawing in it
	draw_in_range(set_draw_range(img_rect, rect()));


	th1.join(); // join background painting thread
}
void PixelArt::paintEvent(QPaintEvent*)
{
	painter.begin(this);
	painter.drawImage(0, 0, canvas);
	//painter.drawImage(back_pos, background);

	if (thick_lines)
	{
		painter.setPen(QPen(Qt::black, 2));
		for (auto& ref : thick_lines_list)
			painter.drawLine(ref.translated(canvas_pos));
	}
	painter.end();
}
// SHORTCUTS
void PixelArt::undo_fn(bool undo)
{
	Cache* sip; Cache* fill;
	if (undo) sip = &undo_cache, fill = &redo_cache;
	else      sip = &redo_cache, fill = &undo_cache;

	if (!sip->empty())
	{
		Cell* temp = &sip->back();
		fill->push_back(
			Cell(
				cell_map[temp->position.first][temp->position.second].color,
				temp->position
			)
		); // to other cache before drawing to preserve initial colour

		update_cell_map(
			temp->position,
			draw_rect(temp->position, temp->color)
		);
		sip->pop_back();
	}
}
void PixelArt::range(int* const val)
{
	if (*val < 0) *val = 0;
}
int PixelArt::range(int val)
{
	if (val > 0) return 0;
	return val;
}
int PixelArt::irange(int val)
{
	if (val < 0) return 0;
	return val;
}
void PixelArt::irange(int* const val)
{
	if (*val < 0) *val = 0;
}
PixelArt::~PixelArt()
{
	canvas.copy(intersected).save("quadratic_pix.png");
}