#include <QStandardItemModel>
#include <QStandardItem>

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "ontologyviewer.h"
#include "ui_ontology_viewer.h"

#include <QMap>

OntologyViewer::OntologyViewer() :
    QWidget(),
    ui(new Ui::OntologyViewer) {

    ui->setupUi(this);

    // Disable the close, minimize, maximize buttons
    QWidget::setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

OntologyViewer::~OntologyViewer() {
    delete ui;
}

void OntologyViewer::loadData(std::string filename) {
  ontology = parse_ontology_json_file(filename);
  
  updateTreeView();
  updateInformableTreeView();
}


void OntologyViewer::updateTreeView() {
    ui->ontology_tree_view->clear();

    ui->ontology_tree_view->setColumnCount(1);
    QList<QTreeWidgetItem*> top_level_items;
    top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Requestable"))));
    QList<QTreeWidgetItem*> requestable_items;
    for(unsigned int i = 0 ; i < ontology.requestable.size() ; ++i)
        requestable_items.append(new QTreeWidgetItem(top_level_items.last(), QStringList(QString("%1").arg(QString::fromStdString(ontology.requestable[i])))));

    top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Methods"))));
    QList<QTreeWidgetItem*> method_items;
    for(unsigned int i = 0 ; i < ontology.method.size() ; ++i)
        method_items.append(new QTreeWidgetItem(top_level_items.last(), QStringList(QString("%1").arg(QString::fromStdString(ontology.method[i])))));

    ui->ontology_tree_view->insertTopLevelItems(0, top_level_items);
}

void OntologyViewer::updateInformableTreeView() {

    ui->ontology_informable_tree_view->clear();
    ui->ontology_informable_tree_view->setColumnCount(1);

    QList<QTreeWidgetItem*> top_level_items;
    QList<QTreeWidgetItem*> bottom_level_items;

    std::map<std::string, std::vector<std::string> >::const_iterator it = ontology.informable.begin();
    std::map<std::string, std::vector<std::string> >::const_iterator it_end = ontology.informable.end();
    for(; it != it_end ; ++it) {
        top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString::fromStdString(it->first))));
        for(unsigned int i = 0 ; i < it->second.size() ; ++i) {
            bottom_level_items.append(new QTreeWidgetItem(top_level_items.last(), QStringList(QString("%1").arg(QString::fromStdString(it->second[i])))));
        }
    }
    ui->ontology_informable_tree_view->insertTopLevelItems(0, top_level_items);
}
