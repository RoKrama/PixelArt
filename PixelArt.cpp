#include "PixelArt.h"
PixelArt::PixelArt(const InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size), // cell vals.cell_size in pix
	q_cell_size(vals.cell_size, vals.cell_size),
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells
	img_pair(QPixmap(vals.cells_x* vals.cell_size, vals.cells_y* vals.cell_size), QPixmap()),
	background(vals.cells_x* vals.cell_size, vals.cells_y* vals.cell_size),
	img_rect(QPoint(0, 0), QSize(x_n* cell_size, y_n* cell_size)),
	intersected(img_rect),
	line_list_x(), line_list_y(),
	canvas_pos(0, 0),
	back_pos(0, 0),
	canvas_offset(0, 0),
	zoom(0), clipped(false),
	painter(), line_pen(Qt::gray, 0),

	undo(QKeySequence::Undo, this),
	redo(QKeySequence::Redo, this),
	move_canvas(QKeySequence(Qt::CTRL | Qt::Key_M), this),
	open_color_dialog(QKeySequence(Qt::CTRL | Qt::Key_C), this),
	show_thick_lines(QKeySequence(Qt::CTRL | Qt::Key_T), this),

	moving_canvas(false), clicked_in_canvas(false),thick_lines(false),
	fib(),
	undo_cache(), redo_cache(),

	color_dialog(QColorDialog(this))
{
	setBackgroundRole(QPalette::AlternateBase);
	setAutoFillBackground(true);
	setFocusPolicy(Qt::StrongFocus);

	constructCanvas();
	constructShortcuts();
}
// CONSTRUCTOR FUNCTIONS
inline void PixelArt::constructCanvas()
{
	canvas = &img_pair.first;
	for (int i = 0; i < x_n; i++)
	{
		cell_map.push_back(std::vector<Cell>());
		for (int j = 0; j < y_n; j++)
			cell_map[i].push_back(Cell(QColor((i + 256) % 255, (j + 256) % 256, (i * j) % 256), i, j));
	}
	line_list_x.reserve(x_n + 1);
	line_list_y.reserve(y_n + 1);
	constructLines();
	fib.push_back(1);
	for (int i = 0; i < 40; i++) fib.push_back(fib[std::max<int>((i - 1),0)] + fib[i]);
	for (auto& ref : fib) ref = std::cbrt(ref);
	
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
			pix::bool_switch(thick_lines);
			update();
		}
	);
	connect(
		&color_dialog, &QColorDialog::currentColorChanged,
		this, [this] { current_color = color_dialog.currentColor(); }
	);
}
inline void PixelArt::constructLines()
{
	// pixel lines
	auto range = [this](int val) -> int
	{
		if (!clipped) return val;
		else return (val - pix::full_mod(val, cell_size));
	};
	std::thread t1(
		[this, &range]() mutable 
		{
			if (!line_list_x.empty()) line_list_x.clear();
			for (
				int i = 0, i_cell_size; 
				(i_cell_size = i * cell_size) <= background.width(); 
				i++
			) line_list_x.push_back(
					QLine(
						i_cell_size, 0,
						i_cell_size, range(background.height())
					)
				);
		}
	);
	std::thread t2(
		[this, &range]() mutable
		{
			if (!line_list_y.empty()) line_list_y.clear();
			for (
				int i = 0, i_cell_size; 
				(i_cell_size = i * cell_size) <= background.height(); 
				i++
			) line_list_y.push_back(
					QLine(
						0, i_cell_size,
						range(background.width()), i_cell_size
					)
				);
		}
	);

	// orientation grid
	if (!thick_lines_list.empty()) thick_lines_list.clear();
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				i * ((x_n * cell_size) / 3), 0,
				i * ((x_n * cell_size) / 3), img_rect.height() + cell_size
			)
		);
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				0, i * ((y_n * cell_size) / 3),
				img_rect.width() + cell_size, i * ((y_n * cell_size) / 3)
			)
		);
	background.fill(Qt::transparent);
	t1.join(); t2.join();
	paintLines();
}
// EVENTS
void PixelArt::wheelEvent(QWheelEvent* event)
{
	QPoint pos(mapFromGlobal(QCursor::pos()));
	if ( // ctrl is held and mouse is in intersected precondition
		GetAsyncKeyState(0x11) >= 0 ||
		!intersected.contains(pos)
		) return;

	int direction;
	bool dir_positive = event->angleDelta().y() > 0;
	if (dir_positive)
	{
		if (zoom > 30) return;
		direction = static_cast<int>(fib[zoom]);
	}
	else direction = static_cast<int>(-fib[std::max<int>((zoom - 1), 0)]);
	if (cell_size + direction < 1) return;

	if (dir_positive) zoom++;
	else if (zoom > 0) zoom--;

	zoomClipFn(direction, pos);
	update();
}
void PixelArt::mousePressEvent(QMouseEvent* event)
{
	if (!img_rect.contains(event->pos())) return;

	clicked_in_canvas = true; // allow moving

	if (moving_canvas) // ctrl + M
	{
		move_pos = event->pos(); // record mouse_pos in class member
		return;	// moving canvas will be handled in mouseMoveEvent
	}
	if (
		event->button() == Qt::MouseButton::LeftButton &&
		!(GetAsyncKeyState(0x11) < 0)
		)
	{
		// getting clicked cell position and storing in std::pair<int, int>
		std::pair<int, int> cell_index = quantise_m_pos(event->pos());
		if (cell_same(cell_index)) return; // check if same colour
		undo_cache.push_front(
			Cell( // constructing cell and loading into undo cache
				cell_map[cell_index.first][cell_index.second].color,
				cell_index
			)
		); // before drawing over
		draw_rect(cell_index, current_color);
	}
}
void PixelArt::mouseMoveEvent(QMouseEvent* event)
{
	// moving of canvas happens if true
	if (!(
		clicked_in_canvas &&
		rect().contains(event->pos())
		)) return;

	if (moving_canvas)
	{
		//t1 = std::chrono::high_resolution_clock::now();
		QPoint move = (event->pos() - move_pos);
		intersected = img_rect.intersected(rect()).adjusted(0, 0, 1, 1);
		img_rect.translate(move);

		set_offset();
		if (clipped) set_bg_clipped_pos();
		else back_pos += move;

		canvas_pos += move;
		move_pos = event->pos();
		update(intersected.translated(move).united(intersected));
	}

	// drawing continously
	else if (img_rect.contains(event->pos()))
	{
		std::pair<int, int> cell_index = quantise_m_pos(event->pos());
		if (cell_same(cell_index)) return;

		undo_cache.push_front(
			Cell( // constructing cell and loading into undo cache
				cell_map[cell_index.first][cell_index.second].color,
				cell_index
			)
		); // before drawing over
		draw_rect(cell_index, current_color);
	}
}
void PixelArt::mouseReleaseEvent(QMouseEvent* event)
{
	clicked_in_canvas = false; // reset
	if (!clipped) return;

	QApplication::processEvents();
	// calling processEvents() is supposed to bad
	// it's the only way I've found to ensure mouseReleaseEvent isn't processed
	// before update() calls from mouseMoveEvent are finished painting
	// otherwise canvas stutters on release with some mouse speed

	set_clipped_canvas_pos();

	QPixmap* other; // construct buffer bitmap
	if (canvas == &img_pair.first) other = &img_pair.second;
	else other = &img_pair.first;
	*other = QPixmap(clipped_canvas_size());

	canvas = other; // set it as main and paint it
	draw_in_range(canvas->rect().translated(canvas_pos));
	update();
}
void PixelArt::keyReleaseEvent(QKeyEvent* event)
{
	if (moving_canvas && event->key() == Qt::Key_M)
		moving_canvas = false; // reset
}
void PixelArt::paintEvent(QPaintEvent*)
{


	painter.begin(this);
	painter.setClipRect(rect());
	painter.drawPixmap(canvas_pos, *canvas);
	painter.drawPixmap(back_pos, background);

	if (thick_lines)
	{
		painter.setPen(QPen(Qt::black, 2));
		for (auto& ref : thick_lines_list)
			painter.drawLine(ref.translated(img_rect.topLeft()));
	}

	if (!clipped)
	{
		painter.setPen(line_pen);
		if (img_rect.right() + 1 < width())
			painter.drawLine(
				img_rect.right() + 1, std::max<int>(img_rect.top(), 0),
				img_rect.right() + 1, std::min<int>(img_rect.bottom(), height())
			);
		if (img_rect.bottom() + 1 < height())
			painter.drawLine(
				std::max<int>(img_rect.left(), 0), img_rect.bottom() + 1,
				std::min<int>(img_rect.right(), width()), img_rect.bottom() + 1
			);
	}
	painter.end();


	//t2 = std::chrono::high_resolution_clock::now();
	//dur = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
	//
	//auto c = [this]() -> std::string { if (clipped) return " clipped"; else return " n_clipped"; };
	//std::cout << dur << c() << std::endl;
}
// CLASS FUNCTIONS
std::pair<int, int> PixelArt::quantise_m_pos(QPoint mouse_pos)
{
	return // get point relative to canvas position
		std::pair<int, int>(
			(mouse_pos.x() - img_rect.left()) / cell_size,
			(mouse_pos.y() - img_rect.top() ) / cell_size
		);
}
QColor PixelArt::draw_rect(const std::pair<int, int>& at, const QColor with)
{
	update_cell_map(at, with);
	QRect area(
		QPoint(
			at.first * cell_size,
			at.second * cell_size
		) + img_rect.topLeft() - canvas_pos,
		q_cell_size
	);

	painter.begin(canvas);
	if (with == Qt::transparent)
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.fillRect(area, with);
	painter.end();

	// clip for faster load
	update(area.translated(canvas_pos));

	return with;
}
bool PixelArt::cell_same(const std::pair<int, int>& at)
{
	return
		cell_map[at.first][at.second].color == current_color;
}
inline void PixelArt::update_cell_map(std::pair<int, int> pos, QColor color)
{
	cell_map[pos.first][pos.second].color = color;
}

inline void PixelArt::zoomClipFn(int dir, const QPoint& from)
{
	//t1 = std::chrono::high_resolution_clock::now();
	cell_size += dir;
	q_cell_size.setWidth(cell_size);
	q_cell_size.setHeight(cell_size);
	int s_x = dir * x_n; // scale size x
	int s_y = dir * y_n; // scale size y

	// making zoom appear happening from cursor
	img_rect.translate( //casting to double before * so it doesnt overflow at large zooms
		static_cast<int>(
			static_cast<double>(-s_x) * (
				static_cast<double>(from.x() - img_rect.left())
				/ static_cast<double>(img_rect.width())
				)
			),
		static_cast<int>(
			static_cast<double>(-s_y) * (
				static_cast<double>(from.y() - img_rect.top())
				/ static_cast<double>(img_rect.height())
				)
			)
	); // yeah...

	img_rect.adjust(0, 0, s_x, s_y); // scale after

	// getting part of rect that should be shown
	intersected = img_rect.intersected(rect());

	set_offset();

	//drawing

	// getting size of image needed to construct
	int w = std::min<int>(img_rect.width(), 3 * width());
	int h = std::min<int>(img_rect.height(), 3 * height());
	// if img_rect is bigger than 3 viewports we can clip it
	// as we won't be able to reach clipped portions of it with one mouse drag
	bool from_clipped = clipped; // if we were clipping in last zoom event the logic is different

	// two background painting lambdas
	auto n_clipped_lambda = [this]
	{
		background = QPixmap(img_rect.size());
		back_pos = img_rect.topLeft();
		constructLines();
	};
	auto clipped_lambda = [this]
	{
		background = QPixmap(size() + q_cell_size); // additional space for movement 
		set_bg_clipped_pos();
		constructLines();
	};
	if (
		// set clipped if it's true we're clipping
		(clipped = (w < img_rect.width() || h < img_rect.height()))
		||
		// it from_clipped is true we can't just copy whole prev since it's still clipped
		from_clipped
		)
	{
		std::thread b_thread;
		if (clipped)
		{
			set_clipped_canvas_pos();
			b_thread = std::thread(clipped_lambda);
		}
		else
		{
			canvas_pos = img_rect.topLeft(); // going from clipped to not
			b_thread = std::thread(n_clipped_lambda); // we need full canvas size lines
		}

		//constructing clipped bitmap
		// if w || h are larger than end side canvas can and should
		// be smaller as not to cover widget background
		*canvas = QPixmap(
			std::min<int>(w, width() + img_rect.right()),
			std::min<int>(h, height() + img_rect.bottom())
		);
		draw_in_range(canvas->rect().translated(canvas_pos));
		b_thread.join();
	}
	else // can just copy whole prev and scale it
	{
		// background and lines painting thread
		std::thread b_thread(n_clipped_lambda);

		*canvas = canvas->scaled(cell_size * x_n, cell_size * y_n);
		canvas_pos = img_rect.topLeft();

		b_thread.join();
	}
}
void PixelArt::draw_in_range(const QRect& show)
{
	int dx_r = x_n - std::max<int>(
		0,
		(((img_rect.right() - show.right()) * x_n)
			/ img_rect.width())
	);
	int dy_r = y_n - std::max<int>(
		0,
		(((img_rect.bottom() - show.bottom()) * y_n)
			/ img_rect.height())
	);
	int dx =
		(std::abs(img_rect.left() - show.left()) * x_n)
		/ img_rect.width();
	int dy =
		(std::abs(img_rect.top() - show.top()) * y_n)
		/ img_rect.height();

	QPoint relative(img_rect.topLeft() - canvas_pos);
	painter.begin(canvas);
	for (int i = dx; i < dx_r; i++)
		for (int j = dy; j < dy_r; j++)
			painter.fillRect(
				QRect(
					QPoint(i * cell_size, j * cell_size) + relative,
					q_cell_size
				),
				cell_map[i][j].color
			);
	painter.end();
}
QSize PixelArt::clipped_canvas_size()
{
	return QSize(
		std::min<int>(
			3 * width(),
			width() + img_rect.right()
			),
		std::min<int>(
			3 * height(),
			height() + img_rect.bottom()
			)
	);
}
inline void PixelArt::set_clipped_canvas_pos()
{
	canvas_pos.setX(std::max<int>(img_rect.left(), -width()));
	canvas_pos.setY(std::max<int>(img_rect.top(), -height()));
}
inline void PixelArt::set_offset()
{
	canvas_offset.setX(img_rect.left() % cell_size);
	canvas_offset.setY(img_rect.top() % cell_size);
}

void PixelArt::paintLines()
{
	line_painter.begin(&background);
	line_painter.setPen(line_pen);
	line_painter.drawLines(line_list_x);
	line_painter.drawLines(line_list_y);
	line_painter.end();
}
void PixelArt::set_bg_clipped_pos()
{
	// back_pos will be limited to one cell size
	// while moving it will jump for a cell size
	// upon moving it over one distance of a cell size

	int leftover;
	if (img_rect.x() >= 0) back_pos.setX(img_rect.x());
	else if (
		canvas_offset.x() > (
			leftover = (
				img_rect.right() - background.width()
				+ pix::full_mod(background.width(), cell_size)
				)
			)
		) back_pos.setX(leftover);
	else back_pos.setX(canvas_offset.x());

	if (img_rect.y() >= 0) back_pos.setY(img_rect.y());
	else if (
		canvas_offset.y() > (
			leftover = (
				img_rect.bottom() - background.height()
				+ pix::full_mod(background.height(), cell_size)
				)
			)
		) back_pos.setY(leftover);
	else back_pos.setY(canvas_offset.y());
}
// SHORTCUTS
void PixelArt::undo_fn(bool undo)
{
	Cache* sip; Cache* fill;
	if (undo) sip = &undo_cache, fill = &redo_cache;
	else      sip = &redo_cache, fill = &undo_cache;

	if (!sip->empty())
	{
		Cell* temp = &sip->front();
		fill->push_front(
			Cell(
				cell_map[temp->position.first][temp->position.second].color,
				temp->position
			)
		); // to other cache before drawing to preserve initial colour
		draw_rect(temp->position, temp->color);
		sip->pop_front();
	}
}

// PUBLIC
void PixelArt::setCanvas_pos()
{
	canvas_pos = rect().center() - canvas->rect().center();
	img_rect.moveTopLeft(canvas_pos);
	intersected = img_rect.intersected(rect());
	back_pos = intersected.topLeft() + canvas_offset;
	draw_in_range(img_rect);
	update();
}
PixelArt::~PixelArt()
{}