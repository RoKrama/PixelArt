#include "InitArt.h"

InitReturn InitArtDialog::initialise()
{
    InitReturn temp;
    InitArtDialog dialog(&temp);
    dialog.exec();
    return temp;
}

InitArtDialog::InitArtDialog(InitReturn* in_r_vals) :
    r_vals(in_r_vals),
    cells_x(0), cells_y(0), cell_size(0),
    create_b(this), //exit_b(this),
    edit_x(this), edit_y(this), edit_s(this),
    line_edits{ &edit_x, &edit_y, &edit_s },
    exit(false)
{
    setPalette(QPalette(QColor(20, 20, 20)));
    setStyleSheet
    (
        "QLineEdit "
        "{"
            "background-color: rgb(30,30,40);"
            "border-color: rgb(242,227,153);"
            "border-width: 1px;"
            "border-style: outset;"
            "border-radius: 2px;"
            "color: rgb(242,227,153);"
        "}"

        "QPushButton"
        "{"
            "color: rgb(242,227,153);"
            "background-color: rgb(50,50,50);"
            "border-radius: 2px;"
            "outline: 0px;"
        "}"
    );

    setGeometry(100, 100, 500, 500);

    create_b.setGeometry(50, 400, 150, 50);
    connect
    (
       &create_b, &QPushButton::clicked,
       this, [&] { close(); }
    );
    exit_b.setGeometry(300, 400, 150, 50);

    for (int i = 0; i < line_edits.size(); i++)
    {
        line_edits[i]->setGeometry(300, 100 + 50 * i, 50, 25);
        line_edits[i]->setAlignment(Qt::AlignRight);
    }
    setModal(true);
    setWindowModality(Qt::WindowModality::ApplicationModal);
    show();
}
InitArtDialog::~InitArtDialog()
{
    *r_vals = InitReturn
    (
        edit_x.text().toInt(),
        edit_y.text().toInt(),
        edit_s.text().toInt()
    );
}