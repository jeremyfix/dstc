#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QString>
#include <QtUiTools/QUiLoader>
#include <QFile>
#include <QMessageBox>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <libgen.h> // basename dirname

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ontology_viewer.show();
    label_viewer.show();
    tracker_output_viewer.show();

    // Action menu for opening the files
    connect(this->ui->actionOpen_ontology, SIGNAL(triggered()), this, SLOT(openOntology()));
    connect(this->ui->actionOpen_dialogs, SIGNAL(triggered()), this, SLOT(openDialogs()));
    connect(this->ui->actionOpen_tracker_output, SIGNAL(triggered()), this, SLOT(openTrackerOutput()));

    // Leave on quit
    connect(this->ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));

    // Show an help message when requested
    connect(this->ui->actionManual, SIGNAL(triggered()), this, SLOT(showTips()));

    // Detecting that a new dialog is selected
    connect(this->ui->dialog_combBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeDialog(QString)));

    // Browsing the dialogs
    connect(this->ui->dialog_prevButton, SIGNAL(clicked()), this, SLOT(prev_dialog()));
    connect(this->ui->dialog_nextButton, SIGNAL(clicked()), this, SLOT(next_dialog()));

    // Using the possibility to filter the dialogs by a prefix
    connect(this->ui->dialog_prefix_lineedit, SIGNAL(textChanged(QString)), this, SLOT(dialog_prefix_textChanged(QString)));

    // Getting the info the tracker output wants to be sync to the mainwindow session id
    connect(&tracker_output_viewer, SIGNAL(sig_sync_to_dialog(bool)), this, SLOT(sync_tracker_output_to_session(bool)));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *) {
    ontology_viewer.close();
    label_viewer.close();
    tracker_output_viewer.close();
}

void MainWindow::openOntology() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open ontology file"), 
						    "~", 
						    tr("JSON Files (*.json)"));
    ontology_viewer.loadData(filename.toStdString());
}

void MainWindow::openDialogs() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open dialogs list file"), 
						    "~", 
						    tr("Dialog list files (*.flist)"));

    this->ui->dialog_prefix_lineedit->setText("");

    // We open and parse the file
    // we save the data in a map with :
    // keys : the unique identifier of the dialog
    // values : the full path of the directory containing the dialogs
    std::ifstream infile(filename.toStdString().c_str());
    if(!infile) {
        std::cerr << "Cannot open dialog filelist " << filename.toStdString() << std::endl;
        return;
    }

    // We fill the map dialog_filelist and the combo box of the ui.
    dialog_filelist.clear();
    this->ui->dialog_combBox->clear();

    std::string line;
    std::string diag_filename;
    std::string diag_fullpath;
    infile >> line;
    while(!infile.eof()){
        infile >> line;
	// basename and dirname can modify the content of their buffer !!
	// so we strdup the char* buffer before calling them
        diag_filename = std::string(basename(strdup(line.c_str())));
        diag_fullpath = std::string(dirname(strdup(filename.toStdString().c_str()))) + std::string("/../../data/") + line ;
        //std::cout << "Filename :" << diag_filename << ";" << diag_fullpath << std::endl;
        dialog_filelist[diag_filename] = diag_fullpath;
        this->ui->dialog_combBox->addItem(QString::fromStdString(diag_filename));
    }
}

void MainWindow::changeDialog(QString value) {

    std::string filename_labels = dialog_filelist[value.toStdString()] + "/label.json";
    if(QFile::exists(QString::fromStdString(filename_labels))) {
        label_viewer.loadData(filename_labels);
        this->ui->labels_fullpath->setText(QString::fromStdString(filename_labels));
        this->ui->labels_fullpath->setEnabled(true);
    }
    else {
        this->ui->labels_fullpath->setText(QString(""));
        this->ui->labels_fullpath->setEnabled(false);
    }

    // And we update the GUI elements for the dialog

    // A new dialog has been selected, we then open the file and parse it
    std::string filename_dialog = dialog_filelist[value.toStdString()] + "/log.json";
    if(QFile::exists(QString::fromStdString(filename_dialog))) {
        this->ui->dialog_fullpath->setText(QString::fromStdString(filename_dialog));
        current_dialog = belief_tracker::parse_dialog_json_file(filename_dialog);

        this->ui->sessions_id_label->setText(QString::fromStdString(current_dialog.session_id));
        this->ui->session_date_label->setText(QString::fromStdString(current_dialog.session_date));
        this->ui->session_time_label->setText(QString::fromStdString(current_dialog.session_time));
        this->ui->caller_id_label->setText(QString::fromStdString(current_dialog.caller_id));
        this->ui->number_of_turns_label->setText(QString("%1").arg(current_dialog.turns.size()));

        // Update the tree view of the turns
        this->ui->dialog_tree_view->clear();

        this->ui->dialog_tree_view->setColumnCount(2);
        QList<QTreeWidgetItem*> top_level_items;
        QList<QTreeWidgetItem*> low_level_items;

        int turn_index = 0;
        for(auto& current_turn : current_dialog.turns) {
            top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Turn #%1").arg(turn_index))));

            // Display the transcript
            QTreeWidgetItem* output_item = new QTreeWidgetItem(top_level_items.last(), QStringList(QString("dialog output")));

            low_level_items.push_back(new QTreeWidgetItem(output_item, QStringList(QString::fromStdString(std::string("Transcript: ") + current_turn.transcript))));
            low_level_items.push_back(new QTreeWidgetItem(output_item, QStringList(QString::fromStdString(std::string("Diag Act  : ") + current_turn.dialog_act))));

            // Display the asr hypothesis
            QTreeWidgetItem* asrhyp_item = new QTreeWidgetItem(top_level_items.last(), QStringList(QString("asr-hyps")));
	    for(auto& asr_hyp: current_turn.asr_hyps) {
	      QStringList column_values;
	      column_values.push_back(QString::fromStdString(asr_hyp.first));
	      column_values.push_back(QString("%1").arg(asr_hyp.second));
	      low_level_items.push_back(new QTreeWidgetItem(asrhyp_item, column_values));
            }


	    // Debug : display the individually extracted slu hyps
	    /*
	    std::cout << "Turn " << turn_index << std::endl;
	    std::cout << "Machine act : " << std::endl;
	    display_dialog_acts(current_turn.machine_acts);
	    std::cout << "User act : " << std::endl;
	    for(auto&  uact : current_turn.user_acts) {
	      if(uact.first.size() == 0) 
		std::cout << "Empty " << std::endl;
	      else
	        display_dialog_acts(uact.first);
	      std::cout << " ; Score : " << uact.second << std::endl << std::endl;
	    }
	    std::cout << std::string(10, '-') << std::endl;
	    */


            // Display the slu hypothesis
            QTreeWidgetItem* sluhyp_item = new QTreeWidgetItem(top_level_items.last(), QStringList(QString("slu-hyps")));

	    for(auto& slu_hyp: current_turn.user_acts) {
                QStringList column_values;
                column_values.push_back(QString::fromStdString(belief_tracker::dialog_acts_to_str(slu_hyp.first)));
                column_values.push_back(QString("%1").arg(slu_hyp.second));
                low_level_items.push_back(new QTreeWidgetItem(sluhyp_item, column_values));
            }

            turn_index++;
        }

        this->ui->dialog_tree_view->insertTopLevelItems(0, top_level_items);
        this->ui->dialog_tree_view->expandAll();
        for(int i = 0 ; i < this->ui->dialog_tree_view->columnCount(); ++i)
            this->ui->dialog_tree_view->resizeColumnToContents(i);
        this->ui->dialog_tree_view->collapseAll();
    }
    else {
        this->ui->dialog_fullpath->setText(QString(""));
    }
}

void MainWindow::prev_dialog() {
    int cur_index = this->ui->dialog_combBox->currentIndex();
    int nb_elements = this->ui->dialog_combBox->count();
    if(nb_elements != 0) {
        int new_element = (cur_index - 1) % nb_elements;
        if(new_element < 0)
            new_element += nb_elements;
        this->ui->dialog_combBox->setCurrentIndex(new_element);
    }
}

void MainWindow::next_dialog() {
    int cur_index = this->ui->dialog_combBox->currentIndex();
    int nb_elements = this->ui->dialog_combBox->count();
    if(nb_elements != 0)
        this->ui->dialog_combBox->setCurrentIndex((cur_index + 1) % nb_elements);
}

void MainWindow::openTrackerOutput() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open tracker output file"), "~", tr("Tracker Output (*.json)"));
    tracker_output_viewer.loadData(filename.toStdString());
}

void MainWindow::sync_tracker_output_to_session(bool sync) {
    if(sync) {
        connect(this->ui->dialog_combBox, SIGNAL(currentIndexChanged(QString)), &tracker_output_viewer, SLOT(new_dialog(QString)));
        tracker_output_viewer.new_dialog(this->ui->dialog_combBox->currentText());
    }
    else
        disconnect(this->ui->dialog_combBox, SIGNAL(currentIndexChanged(QString)), &tracker_output_viewer, SLOT(new_dialog(QString)));
}

void MainWindow::showTips() {
    QMessageBox msgBox;
    QString help_msg;
    help_msg += "You can view different elements :<br>";
    help_msg += "  - the ontology files <br>";
    help_msg += "  - the dialogs and their labels (if they exist) <br>";
    help_msg += "  - the output of a tracker <br>";
    help_msg += "<br>";
    help_msg += " The JSON files you provide must fit the DST challenge format. <br>";
    help_msg += "<br>";
    help_msg += " Some of the structure of the DST challenge must be respected : <br>";
    help_msg += "  - dialogs should be in seperate directories <br>";
    help_msg += "  - these directories must be listed in a file xxx.flist (*)<br>";
    help_msg += "  - each directory contains its dialog in log.json <br>";
    help_msg += "  - if available, labels are given in the same directory as label.json<br>";
    help_msg += " (*) If the filelist file is in path P, when reading a path Q to a dialog in this file we look at P/../../data/Q<br>";
    msgBox.setText(help_msg);
    msgBox.exec();
}

void MainWindow::dialog_prefix_textChanged(QString text) {
  // We filter all the dialog filenames and display in the combo box only the ones for which 
  // text is a prefix
  this->ui->dialog_combBox->clear();
  for(auto& kv: dialog_filelist) 
    if(QString::compare(text, QString::fromStdString(kv.first.substr(0, text.size())), Qt::CaseInsensitive) == 0) 
      this->ui->dialog_combBox->addItem(QString::fromStdString(kv.first));
}
