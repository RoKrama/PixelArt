#include "PixelArt.h"
#include <Windows.h>
PixelArt::PixelArt(const pix::InitReturn vals, QWidget* parent) :
	QWidget(parent),

	cell_size(vals.cell_size, vals.cell_size), // cell vals.cell_size in pix
	x_n(vals.cells_x), y_n(vals.cells_y), // n of cells

	canvas(x_n* vals.cell_size, y_n* vals.cell_size, QImage::Format::Format_ARGB32),
	line_pen(Qt::gray, 2), background(canvas),
	painter(), canvas_pos(0,0),

	undo(QKeySequence::Undo, this),
	redo(QKeySequence::Redo, this),
	move_canvas(QKeySequence(Qt::CTRL | Qt::Key_M), this),
	open_color_dialog(QKeySequence(Qt::CTRL | Qt::Key_C), this),

	moving_canvas(false), clicked_in_canvas(false),
	undo_cache(), redo_cache(),

	color_dialog(QColorDialog(this))
{
	constructCanvas();
	constructShortcuts();
	setFocusPolicy(Qt::StrongFocus);
}

// CONSTRUCTOR FUNCTIONS
inline void PixelArt::constructCanvas()
{
	for (int i = 0; i <= x_n; i++)
		line_list.push_back( 
			QLine(
			i * cell_size.width() , 0,
			i * cell_size.width() , canvas.size().height()
			)
		);
	for (int i = 0; i <= y_n; i++)
		line_list.push_back( 
			QLine(
			0,					   i * cell_size.height() ,
			canvas.size().width(), i * cell_size.height() 
			)
		);

	canvas.fill(Qt::transparent);
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

// CLASS FUNCTIONS
QColor PixelArt::draw_rect(const QPoint at, const QColor with)
{
	painter.begin(&canvas);
	painter.setPen(Qt::NoPen);
	painter.setBrush(with);

	QRect area(
		at.x(), at.y(),
		cell_size.height(), cell_size.width()
	);
	if (with == Qt::transparent) 
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	painter.drawRect(area);

	painter.end();

	update(QRect((at + canvas_pos), cell_size));
	// clip for faster load
	return with;
}

bool PixelArt::quantise_m_pos(QPoint& mouse_pos)
{
	QPoint canvas_mouse = mouse_pos - canvas_pos;
	// get point relative to canvas position

	if (canvas.rect().contains(canvas_mouse))
	{
		mouse_pos = QPoint(
			canvas_mouse - QPoint(
				canvas_mouse.x() % cell_size.height(),
				canvas_mouse.y() % cell_size.width()
			)
		);
		return true;
	}
	else return false;
}
void PixelArt::setCanvas_pos()
{
	canvas_pos = (rect().center() - canvas.rect().center());
	update();
}
void PixelArt::paintLines()
{
	background.fill(Qt::white);
	painter.begin(&background);
	painter.setPen(line_pen);
	painter.drawLines(line_list);
	painter.end();
}

// EVENTS 
void PixelArt::mousePressEvent(QMouseEvent* event)
{
	if (moving_canvas &&  // ctrl + M
		QRect(canvas_pos, canvas.size()).contains(event->pos())) 
	{
		clicked_in_canvas = true; // allow moving
		move_pos = event->pos(); // record mouse_pos in class member
		return;	// moving canvas will be handled in mouseMoveEvent
	}

	if (event->button() == Qt::MouseButton::LeftButton && 
		!(GetAsyncKeyState(0x11) < 0))
	{
		QPoint temp_clicked_pos = (event->pos());
		if (quantise_m_pos(temp_clicked_pos))
		{
			undo_cache.push_back( // before drawing over
				pix::Cell( // constructing cell and loading into undo cache
					temp_clicked_pos,
					canvas.pixelColor(temp_clicked_pos)
				)
			);

			QColor brush = QColor(std::rand() % 255, std::rand() % 255, std::rand() % 255);
			draw_rect(temp_clicked_pos, brush);
		}
	}
}
void PixelArt::mouseMoveEvent(QMouseEvent* event)
{
	if 
	(
		moving_canvas && 
		clicked_in_canvas &&
		QRect(canvas_pos, canvas.size()).contains(event->pos())
	)
	{ // moving of canvas happens if true
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
void PixelArt::paintEvent(QPaintEvent*)
{
	//auto t1 = std::chrono::high_resolution_clock::now();
	painter.begin(this);
	painter.drawImage(canvas_pos, background);
	painter.drawImage(canvas_pos, canvas);
	painter.setPen(line_pen);
	painter.drawRect(
		QRect(
			canvas_pos - QPoint(1, 1),
			canvas.size() + QSize(1, 1)
		) // outer border
	);
	painter.end();

/*	auto t2 = std::chrono::high_resolution_clock::now();
	if (fps != 0) (fps += (float)std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()) /=2;
	else fps += (float)std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()*/;
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
				canvas.pixelColor(temp->first + QPoint(2, 2))
			)
		); // to other cache before drawing to preserve initial colour
		draw_rect(temp->first, temp->second);
		sip->pop_back();
	}
}

PixelArt::~PixelArt()
{
	//std::cout << fps;
}