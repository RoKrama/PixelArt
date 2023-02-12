#include "PixelArt.h"
PixelArt::PixelArt(const InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size), // cell vals.cell_size in pix
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells

	canvas(rect().size(), QImage::Format::Format_ARGB32),
	img_rect(),
	line_list(),
	canvas_pos(0, 0), background(canvas),
	back_pos(0,0),
	canvas_offset(0,0),
	painter(), line_pen(Qt::gray, 1),

	undo			 (QKeySequence::Undo, this),
	redo			 (QKeySequence::Redo, this),
	move_canvas		 (QKeySequence(Qt::CTRL | Qt::Key_M), this),
	open_color_dialog(QKeySequence(Qt::CTRL | Qt::Key_C), this),
	show_thick_lines (QKeySequence(Qt::CTRL | Qt::Key_T), this),

	moving_canvas(false), clicked_in_canvas(false),
	scale_clipping(false), thick_lines(false),
	undo_cache(), redo_cache(),

	color_dialog(QColorDialog(this))
{
	img_rect = canvas.rect();
	for (int i = 0; i < x_n; i++)
	{
		cell_map.push_back(std::vector<Cell>());
		for (int j = 0; j < y_n; j++)
			cell_map[i].push_back(Cell(Qt::white, i, j));
	}
	line_list.reserve(x_n + y_n + 2);
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
	if (!line_list.empty()) line_list.clear();
	// pixel lines
	std::thread t1(
		[this, &background_size]() mutable {

			for (int i = 0; i <= background_size.width() / cell_size; i++)
				line_list.push_back(
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
		line_list.push_back(
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
				canvas.size().height() + cell_size
			)
		);
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				0,
				i * ((y_n * cell_size) / 3),
				canvas.size().width() + cell_size,
				i * ((y_n * cell_size) / 3)
			)
		);
	t1.join();
}

// CLASS FUNCTIONS
QColor PixelArt::draw_rect(const std::pair<int, int>& at, const QColor with)
{
	QPoint position(
		at.first  * cell_size + img_rect.left() - canvas_pos.x(),
		at.second * cell_size + img_rect.top()  - canvas_pos.y()
	);
	QRect area(position, QSize(cell_size, cell_size));

	painter.begin(&canvas);
	if (with == Qt::transparent) 
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(area, with);
	painter.end();

	// clip for faster load
	update(QRect(
			position + canvas_pos,
			QSize(cell_size, cell_size)
		)
	);

	return with;
}

std::pair<int,int> PixelArt::quantise_m_pos(QPoint mouse_pos)
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
	canvas_pos = (rect().center() - canvas.rect().center());
	back_pos = canvas_pos;
	img_rect.moveTopLeft(canvas_pos);
	update();
}
void PixelArt::paintLines()
{
	background.fill(Qt::transparent);
	line_painter.begin(&background);
	line_painter.setPen(line_pen);
	constructLines();
	line_painter.drawLines(QList(line_list));
	line_painter.end();
}
void PixelArt::update_cell_map(std::pair<int, int> pos, QColor color)
{
	cell_map[pos.first][pos.second].color = color;
}
// EVENTS 
void PixelArt::mousePressEvent(QMouseEvent* event)
{
	if (QRect(canvas_pos, canvas.size()).contains(event->pos()))
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
	if (moving_canvas && clicked_in_canvas)
	{	// moving of canvas happens if true
		QPoint move = event->pos();
		canvas_pos += (move - move_pos);
		img_rect.translate(move - move_pos);
		back_pos += (move-move_pos);
		move_pos = move;
		update();
	}
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
		QRect(
			canvas_pos,
			canvas.size()
		).contains(mapFromGlobal(QCursor::pos()))
	) // ctrl is held and mouse is in precondition
	{
		int direction = 0;
		event->angleDelta().y() > 0 ? 
			direction = + 1 : direction = - 1;

		if (cell_size + direction > 1)
		{
			zoomClipFn(direction);
			update();
		}
		event->accept();
	} 
	else event->ignore();
}
void PixelArt::zoomFn(int dir)
{
	auto t1 = std::chrono::high_resolution_clock::now();
	cell_size += dir;
	QSize scale_size(
		cell_size * x_n,
		cell_size * y_n
	);

	std::thread thread1(
		[this, scale_size]() mutable
		{ 
			background = background.scaled(scale_size);
			paintLines(); // irrelevant if canvas is scaled, only background must be
		}
	);
	std::thread thread2(
		[this, scale_size]() mutable
		{ canvas = canvas.scaled(scale_size); }
	);

	// making zoom appear happening from center of the canvas
	canvas_pos.rx() -= ((dir * x_n) / 2);
	canvas_pos.ry() -= ((dir * x_n) / 2);

	thread1.join();
	
	thread2.join();

	auto t2 = std::chrono::high_resolution_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;
} 
void PixelArt::zoomClipFn(int dir)
{
	auto t1 = std::chrono::high_resolution_clock::now();

	cell_size += dir;

	img_rect.adjust(0, 0, dir * x_n, dir * y_n);

	// making zoom appear happening from center of the canvas
	img_rect.translate(-((dir * x_n) / 2), -((dir * y_n) / 2));

	// getting part of rect that should be shown
	QRect intersected = img_rect.intersected(rect());

	// getting data range
	float x_nf = static_cast<float> (x_n);
	float y_nf = static_cast<float> (y_n);
	float i_rw = static_cast<float> (img_rect.width());
	float i_rh = static_cast<float> (img_rect.height());

	int dx_r =  static_cast<int>((static_cast<float>(img_rect.right()  - rect().right())  / i_rw)* x_nf);
	int dy_r =  static_cast<int>((static_cast<float>(img_rect.bottom() - rect().bottom()) / i_rh)* y_nf);
	int dx	 =  static_cast<int>((static_cast<float>(- img_rect.left()) / i_rw) * x_nf) ;
	int dy   =  static_cast<int>((static_cast<float>(- img_rect.top())  / i_rh) * y_nf);

	range(&dx_r); range(&dy_r); range(&dx); range(&dy);

	canvas = QImage(
				 intersected.size(),
				 QImage::Format::Format_ARGB32
			 );

	canvas_offset.setX(range(img_rect.left() % cell_size));
	canvas_offset.setY(range(img_rect.top()  % cell_size));


	std::thread th1( // background and lines painting thread
		[this, &intersected] 
		{ 
			background = QImage(
				intersected.size() - QSize(canvas_offset.x(), canvas_offset.y()),
				QImage::Format::Format_ARGB32
			);
			back_pos = intersected.topLeft() + canvas_offset;
			paintLines();
		}
	);
	canvas_pos = intersected.topLeft();

	painter.begin(&canvas);
	for (int i = 0; (i+ dx)  < (x_n - dx_r); i++)
		for (int j = 0; (j+ dy) < (y_n - dy_r); j++)
			painter.fillRect(
				QRect(
					QPoint(
						i * cell_size,
						j * cell_size
					) + canvas_offset + canvas_pos,
					QSize(cell_size, cell_size)
				),
				cell_map[i+dx][j+dy].color
			);

	painter.end();

	th1.join(); // join background painting thread
	
	auto t2 = std::chrono::high_resolution_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;
}
void PixelArt::paintEvent(QPaintEvent*)
{
	
	painter.begin(this);
	painter.drawImage(0,0, canvas);
	painter.drawImage(back_pos, background);
	painter.setPen(line_pen);
	painter.drawRect( // outer border
		QRect( 
			canvas_pos - QPoint(1, 1),
			canvas.size() + QSize(1, 1)
		)
	);

	painter.setPen(QPen(Qt::red, 3));
	painter.drawRect(img_rect.intersected(rect()));

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
	if (val	> 0) return 0;
	return val;
}
int PixelArt::irange(int val)
{
	if (val < 0) return 0;
	return val;
}
PixelArt::~PixelArt()
{}