#include "dialog.h"

static const char *keyboards_fname = "config/keyboards";
static const char *stops_fname = "config/stops";
static const char *pistons_fname = "config/pistons";
static const char *piston_engaged = "config/last_piston_engaged";
static const char *active_fname = "config/active";
static const char *pedal_color = "background: lightgreen";
static const char *kbd_color[7] = {
    							   "background: red",
    							   "background: sandybrown",
    							   "background: green",
    							   "background: orange",
    							   "background: cyan",
    							   "background: tan",
    							   "background: lightblue"
                                  };
//keyboards, ranks, octaves. 32 bytes represent 32 ranks. 1st rank byte is base (i.e. 64')  2nd byte has bits for additional octaves.
static unsigned char kbd_active_stops[8][32][2]={{{0x00}}};

void Dialog::displayMsg(){
    msgBox.setInformativeText("ERROR");
    msgBox.setText(msgBuf);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

Dialog::Dialog(){
	createMenu();
	scrollArea = new QScrollArea();
    scrollArea->setContentsMargins( 0, 0, 0, 0 );
    scrollArea->setWidgetResizable( true );
    scrollArea->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
	mainLayout = new QHBoxLayout();
	layoutRight = new QGroupBox();
	layoutLeft = new QWidget();
    scrollArea->setWidget(layoutLeft);
	mainLayout->setMenuBar(menuBar);
	mainLayout->addWidget(scrollArea);
	mainLayout->addWidget(layoutRight);
	createGridGroupBox();
	setLayout(mainLayout);
	setWindowTitle(tr("ORGAN SETUP"));
	pistonsLocal = true;

    chkPstnTmr = new QTimer(this);
    // setup signal and slot
    connect(chkPstnTmr, SIGNAL(timeout()), this, SLOT(chkPstnTmrSlot()));

	current_piston=0;	
    chkPstnTmr->start(100);//msec
	FILE *f_keyboards, *f_stops, *f_pistons;

	//	Q_INIT_RESOURCE(draggableicons);

	if(!(f_keyboards = fopen(keyboards_fname, "r"))){
		sprintf(msgBuf, "File missing:  %s", keyboards_fname);
		displayMsg();
		exit(-1); 
	}else{
		//Setup keyboards, pedalboard
		lblPedalboard->setText("PedalBoard");
		lblPedalboard->setStyleSheet(pedal_color);
		keyboardSetup(f_keyboards);
		fclose(f_keyboards);
	}

	if(!(f_stops = fopen(stops_fname, "r"))){
		sprintf(msgBuf, "File missing:  %s", stops_fname);
		displayMsg();
		exit(-1); 
	}else{
		stopSetup(f_stops); 
		fclose(f_stops);
	}

	if(!(f_pistons = fopen(pistons_fname, "r+"))){
		if(!(f_pistons = fopen(pistons_fname, "w"))){
			sprintf(msgBuf, "Can\'t Open File:  %s", pistons_fname);
			displayMsg();
			exit(-1);
		}
	}
	pistonSetup(f_pistons);
	fclose(f_pistons);

	for(int i=0; i<26; i++){
		connect(pbPiston[i], &QPushButton::released, this, &Dialog::handlePbPiston);
	}

	for(int i=0; i<24; i++){
		connect(pbPistonSave[i], &QPushButton::released, this, &Dialog::handlePbPistonSave);
	}
}

void Dialog::chkPstnTmrSlot(){
	FILE *f;
	size_t cnt;
	uint32_t piston;

	if(pistonsLocal)
		return;

    if(!(f = fopen(piston_engaged, "r"))){
        sprintf(msgBuf, "File missing:  %s\n", piston_engaged);
        displayMsg();
        exit(-1); 
    }else{
		cnt = fread(&piston, 4, 1, f);
		if(piston != current_piston && cnt==1)
			for(unsigned char x=0; x<32; x++)
				if(piston & 1<<x)
					pbPiston[x]->animateClick();
        fclose(f);
    }
}

bool Dialog::eventFilter(QObject * obj, QEvent * event){
	QRect r;
	QEvent::Type e;
	
	e = event->type();	
	if(e != QEvent::Enter && e != QEvent::Leave)
		return(false);

	for(int s=0; s<32; s++){
		for(int k=0; k<8; k++){
			r = lstPipeLength[s][k]->geometry();
			if(static_cast<QListWidget *>(obj) == lstPipeLength[s][k] && event->type()==QEvent::Enter){
				//int pressedKey = (static_cast<QKeyEvent*>(event))->key();
				r.setHeight(70);
			}else{
				r.setHeight(40);	
			}
			lstPipeLength[s][k]->setGeometry(r);
		}
	}
	return false;
}

void Dialog::createMenu()
{
	menuBar = new QMenuBar;

	fileMenu = new QMenu(tr("&File"), this);
	pistonCfgAction = fileMenu->addAction(tr("&Pistons Active"));
	exitAction = fileMenu->addAction(tr("E&xit"));
	menuBar->addMenu(fileMenu);

	connect(pistonCfgAction, SIGNAL(triggered()), this, SLOT(&pistonConfig()));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(accept()));
	pistonCfgAction->setCheckable(true);
	pistonCfgAction->setChecked(true);
}

/*void Dialog::lstPipeLengthClickedSlot (QListWidget* itemClicked){
	itemClicked->setFixedSize(180, 80);
	itemClicked->setGeometry(0, 0, 100, 100);
	itemClicked->show();
}*/

/*void Dialog::mouseMoveEvent(QMouseEvent *event){
	QRect r;
	event->accept();	
    QListWidget *child = static_cast<QListWidget*>(childAt(event->pos()));
	if (!child)
		return;
	r = child->geometry();
	r.setHeight(200);
	child->setGeometry(r);
}*/

void Dialog::createGridGroupBox()
{
	char buf[128];

	QGridLayout *layoutScrollGrid = new QGridLayout();
	QGridLayout *gridRight = new QGridLayout();

	lblStops = new QLabel();
    lblStops->setFrameShape(QFrame::Panel);
    lblStops->setFrameShadow(QFrame::Sunken);
    lblStops->setLineWidth(3);
	//lblStops->setStyleSheet("background-color: white; border: 4px inset gray;");
	lblStops->setObjectName(QStringLiteral("lblStops"));
	lblStops->setFixedWidth(210);
	lblStops->setFixedHeight(30);
	lblStops->setAlignment(Qt::AlignCenter);
	lblStops->setText("PIPE RANKS");
	layoutScrollGrid->addWidget(lblStops, 0, 0);

	lblPedalboard = new QLabel();
    lblPedalboard->setFrameShape(QFrame::Panel);
    lblPedalboard->setFrameShadow(QFrame::Sunken);
    lblPedalboard->setLineWidth(3);
	//lblPedalboard->setStyleSheet("background-color: white; border: 4px inset gray;");
	lblPedalboard->setObjectName(QStringLiteral("lblPedalboard"));
	lblPedalboard->setFixedWidth(150);
	lblPedalboard->setFixedHeight(30);
	lblPedalboard->setAlignment(Qt::AlignCenter);
	layoutScrollGrid->addWidget(lblPedalboard, 0, 1);

	for(int i=0; i<7; i++){
		lblKeyboard[i] = new QLabel();
    	lblKeyboard[i]->setFrameShape(QFrame::Panel);
    	lblKeyboard[i]->setFrameShadow(QFrame::Sunken);
    	lblKeyboard[i]->setLineWidth(3);
		//lblKeyboard[i]->setStyleSheet("background-color: white; border: 3px inset gray;");
		sprintf(buf, "lblKeyboard%d", i);
		lblKeyboard[i]->setObjectName(buf);
		lblKeyboard[i]->setFixedWidth(150);
		lblKeyboard[i]->setFixedHeight(30);
		lblKeyboard[i]->setAlignment(Qt::AlignCenter);
		layoutScrollGrid->addWidget(lblKeyboard[i],0, i+2);
	}

	QString qs = "0";
	for(int s=0; s<32; s++){
		for(int k=0; k<8; k++){
			lstPipeLength[s][k] = new QListWidget();
			lstPipeLength[s][k]->installEventFilter(this); 
			lstPipeLength[s][k]->setSelectionMode(QAbstractItemView::ExtendedSelection);
			lstPipeLength[s][k]->setFixedWidth(150);
			lstPipeLength[s][k]->setFixedHeight(50);
			lstPipeLength[s][k]->insertItem(1, qs);
			lstPipeLength[s][k]->setMouseTracking(true);
			lstPipeLength[s][k]->setCurrentRow(0);
			lstPipeLength[s][k]->setStyleSheet("background: lightgray; border: 3px inset gray;");
    		//connect(lstPipeLength[s][k], SIGNAL(itemClicked(QListWidget *)), this, SLOT(lstPipeLengthClickedSlot(QListWidget *)));
			layoutScrollGrid->addWidget(lstPipeLength[s][k], s+1, k+1);
		}
	}

	for(int s=0; s<32; s++){
		pbStop[s] = new QPushButton();
		sprintf(buf, "pbStop%d", s);
		pbStop[s]->setObjectName(buf);
		pbStop[s]->setFixedWidth(210);
		pbStop[s]->setFixedHeight(30);
		layoutScrollGrid->addWidget(pbStop[s], s+1, 0);
	}

	pbPiston[24] = new QPushButton();
	pbPiston[24]->setText("SEND");
	pbPiston[24]->setFixedWidth(100);
	pbPiston[24]->setFixedHeight(30);
	gridRight->addWidget((pbPiston[24]), 0, 1);
	gridRight->addWidget((pbPiston[24]), 0, 1);

	pbPiston[25] = new QPushButton();
	pbPiston[25]->setText("CLEAR");
	pbPiston[25]->setFixedWidth(90);
	pbPiston[25]->setFixedHeight(30);
	gridRight->addWidget((pbPiston[25]), 0, 2);
	gridRight->addWidget((pbPiston[25]), 0, 2);

	for(int p=0; p<24; p++){
		pbPiston[p] = new QPushButton();
		sprintf(buf, "pbPiston%d", p);
		pbPiston[p]->setObjectName(buf);
		sprintf(buf, "Piston%02d", p+1);
		pbPiston[p]->setText(buf);
		pbPiston[p]->setFixedWidth(100);
		pbPiston[p]->setFixedHeight(30);
		pbPiston[p]->setCheckable(true);
		gridRight->addWidget((pbPiston[p]), p+1, 1);
	}

	for(int i=0; i<24; i++){
		pbPistonSave[i] = new QPushButton();
		sprintf(buf, "pbPistonSave%d", i);
		pbPistonSave[i]->setObjectName(buf);
		sprintf(buf, "Save%02d", i+1);
		pbPistonSave[i]->setText(buf);
		pbPistonSave[i]->setFixedWidth(90);
		pbPistonSave[i]->setFixedHeight(30);
		gridRight->addWidget(pbPistonSave[i], i+1, 2);
	}

	layoutScrollGrid->setVerticalSpacing(1);
	layoutScrollGrid->setHorizontalSpacing(1);
	layoutScrollGrid->setMargin(0);
	layoutRight->setLayout(gridRight);
	layoutLeft->setLayout(layoutScrollGrid);
}

void Dialog::pistonConfig(){
	if(pistonCfgAction->isChecked())	
		pistonsLocal = true;
	else
		pistonsLocal = false;
}

void Dialog::stopSetup(FILE *f){
	char buf[128], *str;
	QString qs;
	int j;

	for(int s=0; s<32; s++){
		if(!feof(f)){
			if(fgets(buf, 128, f)){
				pbStop[s]->setText(buf);
				pbStop[s]->setStyleSheet("background: springgreen");
				str = strtok(buf, " \n");
				while(str){
					qs = str;
					if(strcmp(str, "128'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j+1, str);
							allStops[s].octave_flags[k] |= 128;
							if(j==1)//1st size (16', 8'...) found is base size of this rank which needs to be sent to organ kbd/pipe process in "active" file.
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "64'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j+1, str);
							allStops[s].octave_flags[k] |= 64;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "32'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 32;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "16'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 16;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "8'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 8;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "4'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 4;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "2'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 2;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}else if(strcmp(str, "1'") == 0){
						for(int k=0; k<8; k++){
							j = lstPipeLength[s][k]->count();
							lstPipeLength[s][k]->insertItem(j, qs);
							allStops[s].octave_flags[k] |= 1;
							if(j==1)
								kbd_active_stops[k][s][0] = allStops[s].octave_flags[k]; 
						}
					}
					str = strtok(nullptr, " \n");
				}
			}else{
				STOP_CNT=static_cast<unsigned char>(s);
				pbStop[s]->setVisible(false);
				for(int k=0; k<8; k++)
					lstPipeLength[s][k]->setVisible(false);
			}
		}else{
			pbStop[s]->setVisible(false);
			for(int k=0; k<8; k++)
				lstPipeLength[s][k]->setVisible(false);
		}
   } 
}

void Dialog::pistonSetup(FILE *f){
	size_t cnt;

	if(!feof(f)){
		cnt = fread(pistons, sizeof(struct piston), 24, f);
		if(cnt!= 24){
			//Initialize new file
			fwrite(pistons, sizeof(struct piston), 24, f);
		}
	}else{
		fwrite(pistons, sizeof(struct piston), 24, f);
	}
}

void Dialog::keyboardSetup(FILE *f){
	char buf[129];

	for(int i=0; i<7; i++){
		if(!feof(f)){
			if(fgets(buf, 128, f)){
				lblKeyboard[i]->setText(buf);
				lblKeyboard[i]->setStyleSheet(kbd_color[i]);
			}else
				lblKeyboard[i]->setText("");
		}
	}
}

void Dialog::handlePbPistonSave(){
	size_t cnt;
	int indx;
	uint octave;
	QMessageBox msgBox;
	QPushButton* buttonSender = qobject_cast<QPushButton*>(sender()); // retrieve the button you have clicked
	QString buttonText = buttonSender->text(), qTmp;
	QList<QListWidgetItem *>selItems;
	QListWidgetItem *val;

 	indx = buttonText.right(2).toInt();
	if(indx){
		indx -= 1;
	}else{
        msgBox.setText("ERROR");
        msgBox.setInformativeText("ERROR with save button!");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
		return;
	}

	for(int s=0; s<32; s++){
		for(int k=0; k<8; k++){
			selItems = lstPipeLength[s][k]->selectedItems();
			QListIterator<QListWidgetItem*> it(selItems); 
			octave = 0;
			while(it.hasNext()){
				val = it.next();
				qTmp = val->text();
				octave += qTmp.left(qTmp.length()-1).toUInt();
			}
			pistons[indx].kbd[k].octaves[s] = static_cast<unsigned char>(octave);
			if(octave > 0){
				pistons[indx].kbd[k].stops[s/8] |= 1<<(s%8);
			}else{
				pistons[indx].kbd[k].stops[s/8] &= ~(1<<s);
			}
		}
	}

	msgBox.setText("Save Pistons?");
	msgBox.setInformativeText("Do you want to save pistons");
	msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret = msgBox.exec();
	if(ret == QMessageBox::Save){
		FILE *f = fopen(pistons_fname, "w");
		cnt = fwrite(pistons, sizeof(struct piston), 24, f);
		if(cnt!=24){
			msgBox.setText("ERROR");
			msgBox.setInformativeText("ERROR Saving Pistons!!!");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
		fclose(f);
	}
}

void Dialog::handlePbPiston(){
	FILE *f_active;
	char buf[5];
	int indx = 0;//, findIndx;//TODO
	uint octave;
	piston tmpPstn;
    QList<QListWidgetItem *>selItems;
	QList<QListWidgetItem *> items;
    QListWidgetItem *val;
	QPushButton* buttonSender = qobject_cast<QPushButton*>(sender()); // retrieve the button you have clicked
	QString buttonText = buttonSender->text(), qTmp;
	if(buttonText == "CLEAR"){
		indx = 26;//CLEAR button 
	}else if(buttonText == "SEND"){
		indx = 25;//CLEAR button 
	}else{
   		indx = buttonText.right(2).toInt();
	}

	if(indx)
		indx -= 1;

	for(int i=0; i<24; i++){
		if(i != indx){
			pbPiston[i]->setChecked(false);
		}
	}

	if(indx!=24){
		for(int s=0; s<32; s++){
			for(int k=0; k<8; k++){
				lstPipeLength[s][k]->setStyleSheet("background: lightgray; border: 3px inset gray;");
				lstPipeLength[s][k]->clearSelection();
				lstPipeLength[s][k]->setCurrentRow(0);
			}
		}
	}
	
	if(indx==25)
		return;//CLEAR button pushed
	
	if(indx==24){//SEND button pushed, send currently selected ranks to "active" file.
		for(int s=0; s<32; s++){
			for(int k=0; k<8; k++){
                selItems = lstPipeLength[s][k]->selectedItems();
                QListIterator<QListWidgetItem*> it(selItems); 
                octave = 0;
                while(it.hasNext()){
                    val = it.next();
                    qTmp = val->text();
                    octave += qTmp.left(qTmp.length()-1).toUInt();
                }
                tmpPstn.kbd[k].octaves[s] = static_cast<unsigned char>(octave);
                if(octave > 0){
                    tmpPstn.kbd[k].stops[s/8] |= 1<<(s%8);
                }else{
                    tmpPstn.kbd[k].stops[s/8] &= ~(1<<s);
                }
                kbd_active_stops[k][s][1] = tmpPstn.kbd[k].octaves[s];
			}		
		}
	}else{
   		if(pbPiston[indx]->isChecked()){
			for(int i=0; i<32; i++){
				for(int j=0; j<8; j++){
					if(pistons[indx].kbd[j].stops[i/8] & 1<<(i % 8)){
						switch(j){
							case 0:
								lstPipeLength[i][j]->setStyleSheet(pedal_color);
							break;
							default:
								lstPipeLength[i][j]->setStyleSheet(kbd_color[j-1]);
						}
						kbd_active_stops[j][i][1] = pistons[indx].kbd[j].octaves[i];
						for(int k=0; k<8; k++){
							sprintf(buf, "%d", static_cast<int>(pistons[indx].kbd[j].octaves[i] & (1<<k)));
							qTmp = buf;
							if(qTmp != "0"){
								items = lstPipeLength[i][j]->findItems(qTmp, Qt::MatchStartsWith | Qt::MatchCaseSensitive);
								if(items.count() > 0){
									items[0]->setSelected(true);
									qTmp = "0";
									items = lstPipeLength[i][j]->findItems(qTmp, Qt::MatchStartsWith | Qt::MatchCaseSensitive);
									items[0]->setSelected(false);
								}
							}
						}
					}
				}
			}
		}
	}
	if(indx==24 || pbPiston[indx]->isChecked()){
		if(!(f_active = fopen(active_fname, "w+b"))){
			sprintf(msgBuf, "File missing:  %s\n", active_fname);
			displayMsg();
			exit(-1); 
		}else{
			fwrite(kbd_active_stops, 512, 1, f_active);
			fclose(f_active);
		}
	}
}
