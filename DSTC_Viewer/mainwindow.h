#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include "ontologyviewer.h"
#include "labelviewer.h"
#include "trackeroutputviewer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    void closeEvent(QCloseEvent *event);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;

    OntologyViewer ontology_viewer;
    LabelViewer label_viewer;
    TrackerOutputViewer tracker_output_viewer;

    std::map<std::string, std::string> dialog_filelist;
    Dialog current_dialog;
    DialogLabels current_labels;

public slots:
    void openOontology();
    void openDialogs();
    void openTrackerOutput();

    void changeDialog(QString);

    void next_dialog(void);
    void prev_dialog(void);

    void sync_tracker_output_to_session(bool);

    void showTips(void);
};

#endif // MAINWINDOW_H
