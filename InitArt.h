#pragma once
#include "class_declarations.h"
#include <qdialog.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <vector>
#include <qvalidator.h>

class InitArtDialog : public QDialog
{
	Q_OBJECT


	InitArtDialog(const InitArtDialog&) = delete;
	InitArtDialog& operator=(const InitArtDialog&) = delete;

	InitReturn* const r_vals;

	unsigned int cells_x, cells_y, cell_size;

	QPushButton create_b, exit_b;

	QLineEdit edit_x, edit_y, edit_s;
	std::vector<QLineEdit*> line_edits;

	bool exit;


public:
	InitReturn static initialise();
	InitArtDialog(InitReturn*) ;
	~InitArtDialog();
};
