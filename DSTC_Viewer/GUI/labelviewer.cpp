#include "labelviewer.h"
#include "ui_labelviewer.h"

LabelViewer::LabelViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LabelViewer)
{
    ui->setupUi(this);

    // Disable the close, minimize, maximize buttons
    QWidget::setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

LabelViewer::~LabelViewer()
{
    delete ui;
}

void LabelViewer::loadData(std::string filename) {
    labels = parse_label_json_file(filename);
    updateView();
}

void LabelViewer::updateView() {

    // Task-information
    // Goal
    QString goal_text("");
    goal_text += "<b>Goal task description : </b>" + QString::fromStdString(labels.task_text) + "<br>";
    goal_text += "<b>Constraints : </b>" + QString::fromStdString(labels.constraints) + "<br>";
    goal_text += "<b>Request slots </b>" + QString::fromStdString(labels.request_slots) + "<br>";
    this->ui->label_goal_textedit->setText(goal_text);

    // Feedback
    this->ui->label_is_successfull_label->setText(QString::fromStdString(labels.was_successfull ? "Yes" : "No"));
    this->ui->label_questionnaire_textedit->setText("<b>Comments : </b><br>" + QString::fromStdString(labels.comments) + "<br>"
                                                    + "<b>Questionnaire :</b><br>" + QString::fromStdString(labels.questionnaire));


    // Turns
    this->ui->label_turns_tree_view->setColumnCount(1);
    QList<QTreeWidgetItem*> top_level_items;
    QList<QTreeWidgetItem*> low_level_items;
    for(auto& current_turn : labels.turns) {
        top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Turn #%1").arg(current_turn.turn_index))));
        QTreeWidgetItem* current_item =  top_level_items.last();
        low_level_items.push_back(new QTreeWidgetItem(current_item, QStringList(QString::fromStdString(std::string("Transcription: ") + current_turn.transcription))));
        low_level_items.push_back(new QTreeWidgetItem(current_item, QStringList(QString::fromStdString(std::string("Semantics: ") + current_turn.semantics))));
        low_level_items.push_back(new QTreeWidgetItem(current_item, QStringList(QString::fromStdString(std::string("Goal labels: ") + current_turn.goal_labels_str))));
        low_level_items.push_back(new QTreeWidgetItem(current_item, QStringList(QString::fromStdString(std::string("Method label: ") + current_turn.method_label))));
        low_level_items.push_back(new QTreeWidgetItem(current_item, QStringList(QString::fromStdString(std::string("Requested slots: ") + current_turn.requested_slots_str))));
    }

    this->ui->label_turns_tree_view->clear();
    this->ui->label_turns_tree_view->insertTopLevelItems(0, top_level_items);
    this->ui->label_turns_tree_view->expandAll();
    for(int i = 0 ; i < this->ui->label_turns_tree_view->columnCount(); ++i)
        this->ui->label_turns_tree_view->resizeColumnToContents(i);
    this->ui->label_turns_tree_view->collapseAll();
}


