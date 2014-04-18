#ifndef LABELVIEWER_H
#define LABELVIEWER_H

#include <QWidget>
#include "JSONObjects.h"

namespace Ui {
class LabelViewer;
}

class LabelViewer : public QWidget
{
    Q_OBJECT
    
public:
    explicit LabelViewer(QWidget *parent = 0);
    ~LabelViewer();

    void loadData(std::string filename);
    
private:
    Ui::LabelViewer *ui;
    DialogLabels labels;

    void updateView();
};

#endif // LABELVIEWER_H
