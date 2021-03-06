#include "trackeroutputviewer.h"
#include "ui_trackeroutputviewer.h"

TrackerOutputViewer::TrackerOutputViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrackerOutputViewer) {

    ui->setupUi(this);

    connect(this->ui->tracker_output_sync_to_session, SIGNAL(toggled(bool)), this, SLOT(sync_to_dialog(bool)));
    connect(this->ui->tracker_output_session_id_combobox, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateTreeView(QString)));
    connect(this->ui->tracker_output_prev_session, SIGNAL(clicked()), this, SLOT(prev_output()));
    connect(this->ui->tracker_output_next_session, SIGNAL(clicked()), this, SLOT(next_output()));

    // Disable the close, minimize, maximize buttons
    QWidget::setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

TrackerOutputViewer::~TrackerOutputViewer() {
    delete ui;
}

void TrackerOutputViewer::loadData(std::string filename) {
  tracker_output = belief_tracker::parse_tracker_output_json_file(filename);
    this->ui->tracker_output_filename->setText(QString::fromStdString(filename));
    updateView();

    // If the tracker is already in sync mode, we request to be sync again
    emit sig_sync_to_dialog(this->ui->tracker_output_sync_to_session->isChecked());
}

void TrackerOutputViewer::updateView() {
    this->ui->tracker_output_dataset->setText(QString::fromStdString(tracker_output.dataset));
    this->ui->tracker_output_walltime->setText(QString("%1").arg(tracker_output.walltime));

    this->ui->tracker_output_session_id_combobox->clear();

    for(auto& s: tracker_output.sessions_turns)
        this->ui->tracker_output_session_id_combobox->addItem(QString::fromStdString(s.first));
}

void TrackerOutputViewer::updateTreeView(QString cur_session_id) {

    this->ui->tracker_output_tree_view->clear();

    //auto current_session = tracker_output.sessions_turns.find(cur_session_id.toStdString());
    auto current_session = std::find_if(tracker_output.sessions_turns.begin(), 
					tracker_output.sessions_turns.end(),
					[cur_session_id] (std::pair<std::string, std::vector<belief_tracker::TrackerSessionTurn> > const &p) -> bool { return p.first.compare(cur_session_id.toStdString()) == 0;});
    if(current_session != tracker_output.sessions_turns.end()) {
        // We received a valid session-id, i.e. one appearing
        // in the tracker_output
        this->ui->tracker_output_tree_view->setColumnCount(1);
        QList<QTreeWidgetItem*> top_level_items;
        QList<QTreeWidgetItem*> low_level_items;

        for(auto & cturn : current_session->second) {
            top_level_items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Turn #%1").arg(cturn.turn_index))));
            QTreeWidgetItem* top_level = top_level_items.last();

            // Goal labels
            low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(QString("Goal labels : "))));
            for(auto & kv : cturn.goal_labels_scores) {
	      QString str(" - ");
	      str += QString::fromStdString(kv.first); // put the slot name
	      str += QString("(");
	      unsigned int nb_elements_vscore = kv.second.size();
	      unsigned int index_vscore= 0;
	      for(auto& vscore: kv.second) {
		str += QString::fromStdString(vscore.first) + QString(":%1").arg(vscore.second);
		if(index_vscore < nb_elements_vscore - 1)
		  str += QString(";");
		++index_vscore;
	      }
	      str += QString(")");
	      low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(str)));
	    }

            // Goal labels joint
            low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(QString("Goal labels joint: "))));
            for(auto& kv: cturn.goal_labels_joint_scores) {
	      double score_joint = kv.second;
	      QString str(" - ");
	      for(auto& sv: kv.first)
		str += QString::fromStdString(sv.first) + QString("=") + QString::fromStdString(sv.second) + QString(" ");
	      str += QString(" ; score : %1").arg(score_joint);
	      low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(str)));
	    }

            // Method label
	    QString str_method("Method label : ");
	    for(auto& kv: cturn.method_labels_scores)
	      str_method += QString::fromStdString(kv.first) + QString("(") + QString("%1").arg(kv.second) + QString(") ");
	    
            low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(str_method)));

            // Requested slots
	    QString str_requested("Requested slots : ");
	    for(auto& kv: cturn.requested_slots_scores)
	      str_requested += QString::fromStdString(kv.first) + QString("(") + QString("%1").arg(kv.second) + QString(") ");
            low_level_items.push_back(new QTreeWidgetItem(top_level, QStringList(str_requested)));

        }

        this->ui->tracker_output_tree_view->insertTopLevelItems(0, top_level_items);
        this->ui->tracker_output_tree_view->expandAll();
        for(int i = 0 ; i < this->ui->tracker_output_tree_view->columnCount(); ++i)
            this->ui->tracker_output_tree_view->resizeColumnToContents(i);
        this->ui->tracker_output_tree_view->collapseAll();

        // Clean up the status bar
        this->ui->tracker_output_status_label->setText(QString(""));

    }
    else {
        this->ui->tracker_output_status_label->setText(QString("Cannot find the key ") + cur_session_id);
    }


}

void TrackerOutputViewer::sync_to_dialog(bool sync) {
    // Disable the manual browsing of the outputs
    this->ui->tracker_output_session_id_combobox->setEnabled(!sync);
    this->ui->tracker_output_prev_session->setEnabled(!sync);
    this->ui->tracker_output_next_session->setEnabled(!sync);

    // Indicate we want to be sync.
    emit sig_sync_to_dialog(sync);
}

void TrackerOutputViewer::new_dialog(QString session_id) {
    //std::cout << "Tracker output viewer received " << session_id.toStdString() << std::endl;
    int new_index = this->ui->tracker_output_session_id_combobox->findText(session_id);
    if(new_index != -1) {
        this->ui->tracker_output_session_id_combobox->setCurrentIndex(new_index);
        // setting the current index automatically triggers
        // the update of the tree view
        this->ui->tracker_output_status_label->setText(QString(""));
    }
    else {
        this->ui->tracker_output_status_label->setText(QString("Cannot find the key ") + session_id);
    }
}

void TrackerOutputViewer::prev_output() {
    int nb_elements = this->ui->tracker_output_session_id_combobox->count();
    if(nb_elements != 0) {
        int cur_index = this->ui->tracker_output_session_id_combobox->currentIndex();
        int new_index = (cur_index - 1) % nb_elements;
        if(new_index < 0)
            new_index += nb_elements;
        this->ui->tracker_output_session_id_combobox->setCurrentIndex(new_index);
    }
}

void TrackerOutputViewer::next_output() {
    int nb_elements = this->ui->tracker_output_session_id_combobox->count();
    if(nb_elements != 0) {
        int cur_index = this->ui->tracker_output_session_id_combobox->currentIndex();
        int new_index = (cur_index + 1) % nb_elements;
        this->ui->tracker_output_session_id_combobox->setCurrentIndex(new_index);
    }

}
