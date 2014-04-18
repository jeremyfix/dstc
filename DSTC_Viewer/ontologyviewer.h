#ifndef ONTOLOGYVIEWER_H
#define ONTOLOGYVIEWER_H

#include <QWidget>
#include <vector>
#include <string>
#include <map>
#include "JSONObjects.h"

namespace Ui {
    class OntologyViewer; // <-- this is the automatically created ui class which we need
}

class OntologyViewer : public QWidget
{
    Q_OBJECT

public:
    OntologyViewer();
    ~OntologyViewer();

    void loadData(std::string filename);

private:

    void updateTreeView();
    void updateInformableTreeView();

    Ui::OntologyViewer *ui;
    Ontology ontology;
};

#endif // ONTOLOGYVIEWER_H
