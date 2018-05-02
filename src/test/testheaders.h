/*
 Author: Ange Optimization <contact@ange.dk>  (C) Ange Optimization ApS 2010

 Copyright: See COPYING file that comes with this distribution

*/

#ifndef TESTHEADERS_H
#define TESTHEADERS_H

#include <QObject>
#include "danishnamecube.h"

class QAction;
class QPoint;
class QLayout;
class QStandardItemModel;
class QTableView;
namespace qdatacube {
class datacube_model_t;
class DatacubeView;
}

class QAbstractItemModel;
class testheaders : public danishnamecube_t {
  Q_OBJECT
  public:
    void createtableview();
    testheaders(QObject* parent = 0);
  public Q_SLOTS:
    void slot_set_model();
    void slot_set_filter();
    void slot_set_data();
    void slot_insert_data();
    void slot_remove_data();
    void slot_filter_button_pressed();

    void slot_horizontal_context_menu(const QPoint& pos, int headerno, int category);
    void slot_vertical_context_menu(const QPoint& pos, int headerno, int category);
    void summarize_weight();
  private:
    qdatacube::Datacube* m_datacube;
    qdatacube::DatacubeView* m_view;
    void add_filter_button(qdatacube::AbstractAggregator::Ptr filter, QLayout* layout);
    QAction* create_aggregator_action(qdatacube::AbstractAggregator::Ptr aggregator);
    QAction* m_collapse_action;
    QList<QAction*> m_col_used_aggregator_actions;
    QList<QAction*> m_row_used_aggregator_actions;
    QList<QAction*> m_unused_aggregator_actions;
    QTableView* m_underlying_table_view;

};


#endif // TESTHEADERS_H
