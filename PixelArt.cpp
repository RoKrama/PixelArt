#include "PixelArt.h"
PixelArt::PixelArt(const InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size), // cell vals.cell_size in pix
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells

	canvas(x_n * cell_size, y_n * cell_size, QImage::Format::Format_ARGB32),
	canvas_pos(0, 0), background(canvas),
	sc_canvas(), sc_background(),
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
	// pixel lines
	if (!line_list.empty()) line_list.clear();
	for (int i = 0; i <= x_n; i++)
		line_list.push_back(
			QLine(
				i * cell_size, 0,
				i * cell_size, canvas.size().height()
			)
		);
	for (int i = 0; i <= y_n; i++) 
		line_list.push_back(
			QLine(
				0,					   i * cell_size,
				canvas.size().width(), i * cell_size
			)
		);

	// orientation grid
	if (!thick_lines_list.empty()) thick_lines_list.clear();
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				i * ((x_n * cell_size) / 3), 0,
				i * ((x_n * cell_size) / 3), canvas.size().height()
			)
		);
	for (int i = 1; i <= 2; i++)
		thick_lines_list.push_back(
			QLine(
				0,					   i * ((y_n * cell_size) / 3),
				canvas.size().width(), i * ((y_n * cell_size) / 3)
			)
		);
}

// CLASS FUNCTIONS
QColor PixelArt::draw_rect(const QPoint at, const QColor with)
{
	painter.begin(&canvas);

	QRect area(
		at.x(), at.y(),
		cell_size, cell_size
	);
	if (with == Qt::transparent) 
		painter.setCompositionMode(QPainter::CompositionMode_Clear);

	painter.fillRect(area, with);

	painter.end();

	// clip for faster load
	update(QRect((at + canvas_pos), QSize(cell_size, cell_size)));

	return with;
}

void PixelArt::quantise_m_pos(QPoint& mouse_pos)
{
	QPoint canvas_mouse = mouse_pos - canvas_pos;
	// get point relative to canvas position
		mouse_pos = QPoint(
			canvas_mouse - QPoint(
				canvas_mouse.x() % cell_size,
				canvas_mouse.y() % cell_size
			)
		);
}
void PixelArt::setCanvas_pos()
{
	canvas_pos = (rect().center() - canvas.rect().center());
	update();
}
void PixelArt::paintLines()
{
	background.fill(Qt::transparent);
	painter.begin(&background);
	painter.setPen(line_pen);
	constructLines();
	painter.drawLines(line_list);
	painter.end();
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
			QPoint temp_clicked_pos = (event->pos());
			quantise_m_pos(temp_clicked_pos);
			undo_cache.push_back( // before drawing over
				Cell( // constructing cell and loading into undo cache
					canvas.pixelColor(temp_clicked_pos),
					std::pair<int, int>(
						temp_clicked_pos.x() / cell_size,
						temp_clicked_pos.y() / cell_size
					)
				)
			);
			draw_rect(temp_clicked_pos, current_color);
		}
	}
}
void PixelArt::mouseMoveEvent(QMouseEvent* event)
{
	if (moving_canvas && clicked_in_canvas)
	{	// moving of canvas happens if true
		canvas_pos += (event->pos() - move_pos);
		move_pos = event->pos();
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
		if (event->angleDelta().y() > 0) direction = + 1;
		else direction = - 1;

		if (cell_size + direction > 1)
		{
			zoomFn(direction);
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
		{ background = background.scaled(scale_size); }
	);
	std::thread thread2(
		[this, scale_size]() mutable
		{ canvas = canvas.scaled(scale_size); }
	);

	// making zoom appear happening from center of the canvas
	canvas_pos.rx() -= ((dir * x_n) / 2);
	canvas_pos.ry() -= ((dir * x_n) / 2);

	thread1.join();
	paintLines(); // irrelevant if canvas is scaled, only background must be
	thread2.join();

	auto t2 = std::chrono::high_resolution_clock::now();
	calc1 += (((float)std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count())
		/ ((float)(scale_size.height())) );
} 
void PixelArt::zoomClipFn(int dir)
{}
void PixelArt::paintEvent(QPaintEvent*)
{
	
	painter.begin(this);
	painter.drawImage(canvas_pos, canvas);
	painter.drawImage(canvas_pos, background);
	painter.setPen(line_pen);
	painter.drawRect( // outer border
		QRect( 
			canvas_pos - QPoint(1, 1),
			canvas.size() + QSize(1, 1)
		)
	);

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
	undo ? sip = &undo_cache, fill = &redo_cache
	     : sip = &redo_cache, fill = &undo_cache;

	if (!sip->empty())
	{
		Cell* temp = &sip->back();
		fill->push_back(
			Cell(
				canvas.pixelColor(
					QPoint(
						temp->position.first  * cell_size, 
						temp->position.second * cell_size
					)
				),
				temp->position 
			)
		); // to other cache before drawing to preserve initial colour
		draw_rect(
			QPoint(
				temp->position.first  * cell_size,
				temp->position.second * cell_size
			), 
			temp->color
		);
		sip->pop_back();
	}
}
PixelArt::~PixelArt()
{
	std::cout << calc1;
}