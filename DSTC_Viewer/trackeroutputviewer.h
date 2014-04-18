#ifndef TRACKEROUTPUTVIEWER_H
#define TRACKEROUTPUTVIEWER_H

#include <QWidget>
#include <QString>
#include "JSONObjects.h"

namespace Ui {
class TrackerOutputViewer;
}

class TrackerOutputViewer : public QWidget
{
    Q_OBJECT
    
public:
    explicit TrackerOutputViewer(QWidget *parent = 0);
    ~TrackerOutputViewer();

    void loadData(std::string filename);
    
signals:
    void sig_sync_to_dialog(bool);

public slots:
    void new_dialog(QString);
    void sync_to_dialog(bool);

    // Updates the tree view given a session-id
    void updateTreeView(QString);

    // Slots to go the previous or next tracker output
    // when the viewer is not sync. to the mainwindow session-id
    void prev_output();
    void next_output();

private:
    Ui::TrackerOutputViewer *ui;
    TrackerOutput tracker_output;

    // Method to update the walltime, dataset and combo box elements
    void updateView(void);

};

#endif // TRACKEROUTPUTVIEWER_H
