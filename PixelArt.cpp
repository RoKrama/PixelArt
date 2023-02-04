#include "PixelArt.h"
#include <Windows.h>
#include <thread>
PixelArt::PixelArt(const pix::InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size), // cell vals.cell_size in pix
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells

	canvas(x_n * cell_size, y_n * cell_size, QImage::Format::Format_ARGB32),
	canvas_pos(0, 0), background(canvas),
	painter(), line_pen(Qt::gray, 1),

	undo			 (QKeySequence::Undo, this),
	redo			 (QKeySequence::Redo, this),
	move_canvas		 (QKeySequence(Qt::CTRL | Qt::Key_M), this),
	open_color_dialog(QKeySequence(Qt::CTRL | Qt::Key_C), this),

	moving_canvas(false), clicked_in_canvas(false),
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
}
inline void PixelArt::constructLines() 
{
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

	update(QRect((at + canvas_pos), QSize(cell_size, cell_size)));
	// clip for faster load
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
				pix::Cell( // constructing cell and loading into undo cache
					temp_clicked_pos,
					canvas.pixelColor(temp_clicked_pos)
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
	//auto t1 = std::chrono::high_resolution_clock::now();

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

	// making cache reference points relevant for new image size
	for (pix::Cell& cell : undo_cache)
	{
		cell.first.rx() += (dir * (cell.first.x() / (cell_size - dir)));
		cell.first.ry() += (dir * (cell.first.y() / (cell_size - dir)));
	}
	for (pix::Cell& cell : redo_cache)
	{
		cell.first.rx() += (dir * (cell.first.x() / (cell_size - dir)));
		cell.first.ry() += (dir * (cell.first.y() / (cell_size - dir)));
	}

	// making zoom appear happening from center of the canvas
	canvas_pos.rx() -= ((dir * x_n) / 2);
	canvas_pos.ry() -= ((dir * x_n) / 2);

	thread1.join();
	paintLines(); // irrelevant if canvas is scaled, only background must be
	thread2.join();
	
	//auto t2 = std::chrono::high_resolution_clock::now();
	//std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << std::endl;
} 
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
	painter.end();
}
// SHORTCUTS
void PixelArt::undo_fn(bool undo)
{
	pix::Cache* sip, * fill;
	if (undo) sip = &undo_cache, fill = &redo_cache;
	else      sip = &redo_cache, fill = &undo_cache;

	if (!sip->empty())
	{
		pix::Cell* temp = &sip->back();
		fill->push_back(
			pix::Cell(
				temp->first,
				canvas.pixelColor(temp->first)
			)
		); // to other cache before drawing to preserve initial colour
		draw_rect(temp->first, temp->second);
		sip->pop_back();
	}
}
PixelArt::~PixelArt()
{
	QImage save_img(background);
	painter.begin(&save_img);
	painter.drawImage(0,0,canvas);
	painter.end();
	save_img.save("heart.png");
	canvas.save("trsp_heart.png");
}