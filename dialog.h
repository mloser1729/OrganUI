#ifndef DIALOG_H
#define DIALOG_H

#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QListIterator>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QtWidgets>
#include <QTimer>
#include <math.h>
#include <stdio.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE
class QAction;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QMenu;
class QMenuBar;
class QPushButton;
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

    struct stop{
    	unsigned char octave_flags[8] = {0};//One byte of flags for all 8 key/pedal boards.  64', 32', 16', ... 1' as bits
    };

    struct keyboard{
		unsigned char stops[4]={0};
 		unsigned char octaves[32]={0};
    };
    
    struct piston{
        struct keyboard kbd[8];    
    };
    
public:
    Dialog();
	bool eventFilter(QObject * obj, QEvent * event) override;
    void keyboardSetup(FILE *f);
	void stopSetup(FILE *f);
	void pistonSetup(FILE *f);
	void handlePbPiston();
	void handlePbPistonSave();
	void handleCboPipeLength(int pstn, int stp, int kbd);
    void lstPipeLengthClickedSlot (QListWidget* itemClicked);
	void displayMsg();
	void pistonCheckLoop();
	void pistonConfig();
	//void mousePressEvent(QMouseEvent *event);
	//void mouseMoveEvent(QMouseEvent *event) override;

    enum { NumGridRows = 3, NumButtons = 4 };

    QHBoxLayout *mainLayout;
	QGroupBox *layoutRight;
	QWidget *layoutLeft;
    QLabel *lblStops, *lblPedalboard, *lblKeyboard[7], *labels[NumGridRows];
    QPushButton *pbPiston[26], *pbPistonSave[24], *pbStop[32], *buttons[NumButtons];
    QListWidget *lstPipeLength[32][8];
    QMenuBar *menuBar;
    QMenu *menuFile, *menuEdit, *fileMenu;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
	QScrollArea *scrollArea;
    QAction *exitAction;
    QAction *pistonCfgAction;
	QMessageBox msgBox;
	QTimer *chkPstnTmr;
	char msgBuf[1024];
	unsigned char STOP_CNT;
	uint32_t current_piston;
	bool pistonsLocal;

public slots:
	void chkPstnTmrSlot();

private:
    void createMenu();
    void createHorizontalGroupBox();
    void createGridGroupBox();
    void createFormGroupBox();

    piston pistons[24];
    stop allStops[32];
};

#endif // DIALOG_H
