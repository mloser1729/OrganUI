#include <QApplication>
#include "dialog.h"
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFont font("Verdana"); //"Courier New");
    font.setStyleHint(QFont::Monospace);
	font.setPointSize(9);
	font.setBold(true);
    QApplication::setFont(font);


    Dialog dialog;
    QRect r = QApplication::desktop()->availableGeometry();
	    int l,t,w,h;
	    l = t = 10;
	    w = r.width() - 2*l;
	    h = r.height() - 2*t;
	    dialog.setGeometry(l,t,w,h);

    //dialog.show();
    dialog.showMaximized();

    return app.exec();
}
