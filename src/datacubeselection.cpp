#include "datacubeselection.h"
#include "datacube.h"
#include <QVector>
#include "datacubeview.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <QSet>
#include "cell.h"
#include "datacube_p.h"
#include "datacubeselection_p.h"
#include <QAbstractProxyModel>
#include <QDate>
#include <QItemSelectionModel>

namespace qdatacube {


QList< int > DatacubeSelectionPrivate::elements_from_selection(QItemSelection selection) {
  QList<int> rv;
  if (synchronized_selection_model) {
    // Create list of rows (that is, indexes of the first column)
    QList<QModelIndex> indexes;
    const QAbstractItemModel* model = synchronized_selection_model->model();
    Q_FOREACH(QItemSelectionRange range, selection) {
      Q_ASSERT(synchronized_selection_model->model() == range.model());
      for (int row=range.top(); row<=range.bottom(); ++row) {
        indexes << model->index(row, 0);
      }
    }

    // Map that list all the way back to the source model
    while(const QAbstractProxyModel* proxy = qobject_cast<const QAbstractProxyModel*>(model)) {
      QList<QModelIndex> mapped_indexes;
      Q_FOREACH(const QModelIndex& unmapped_index, indexes) {
        mapped_indexes << proxy->mapToSource(unmapped_index);
      }
      indexes = mapped_indexes;
      model = proxy->sourceModel();
    }
    // Check that we did indeed get back to the underlying model
    if (model == datacube->underlyingModel()) {
      // Finally, convert to elements = rows in underlying model
      Q_FOREACH(const QModelIndex& index, indexes) {
        rv << index.row();
      }
    } else {
      qWarning("Unable to map selection to underlying model");
    }
  }
  return rv;

}

QItemSelection DatacubeSelectionPrivate::map_to_synchronized(QList< int > elements) {
  QItemSelection selection;
  if (synchronized_selection_model) {
    // Get the reversed list of proxies to source model
    const QAbstractItemModel* model = synchronized_selection_model->model();
    const QAbstractItemModel* underlying_model = datacube->underlyingModel();
    QList<const QAbstractProxyModel*> proxies;
    while (model != underlying_model) {
      if (const QAbstractProxyModel* proxy = qobject_cast<const QAbstractProxyModel*>(model)) {
        proxies << proxy;
        model = proxy->sourceModel();
      } else {
        qWarning("Unable to select on synchronized model");
        return QItemSelection();
      }
    }
    std::reverse(proxies.begin(), proxies.end());

    QList<int> rows = elements;

    // Map from source to whatever proxy model the synchronized model
    const QAbstractItemModel* last_model = model;
    QList<int> proxy_rows;
    Q_FOREACH(const QAbstractProxyModel* proxy, proxies) {
      proxy_rows.clear();
      Q_FOREACH(int row, rows) {
        proxy_rows << proxy->mapFromSource(last_model->index(row,0)).row();
      }
      rows = proxy_rows;
      last_model = proxy;
    }
    qSort(rows);
    const int last_column = model->columnCount()-1;
    model = synchronized_selection_model->model();
    if (rows.size()>0) {
      int lastrow = rows.front();
      int start = lastrow;
      for (int i=1, iend=rows.size(); i<iend; ++i) {
        ++lastrow;
        const int row = rows.at(i);
        if ( row != lastrow) {
          selection << QItemSelectionRange(model->index(start,0), model->index(lastrow-1, last_column));
          start = row;
          lastrow = row;
        }
      }
      selection << QItemSelectionRange(model->index(start,0), model->index(lastrow, last_column));
    }
  }
  return selection;
}

void DatacubeSelectionPrivate::select_on_synchronized(QList<int> elements) {
  if (elements.isEmpty()) {
    return;
  }
  if (synchronized_selection_model && !ignore_synchronized) {
    // Select on sync. model
    QItemSelection selection(map_to_synchronized(selected_elements.toList()));
    ignore_synchronized = true;
    synchronized_selection_model->select(selection, QItemSelectionModel::ClearAndSelect);
    ignore_synchronized = false;
  }

}

void DatacubeSelectionPrivate::deselect_on_synchronized(QList< int > elements) {
  if (synchronized_selection_model) {
    // Select on sync. model
    ignore_synchronized = true;
    synchronized_selection_model->select(map_to_synchronized(elements), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    ignore_synchronized = false;
  }

}
void DatacubeSelectionPrivate::clear_synchronized() {
  if (synchronized_selection_model) {
    ignore_synchronized = true;
    synchronized_selection_model->clearSelection();
    ignore_synchronized = false;
  }
}

DatacubeSelectionPrivate::DatacubeSelectionPrivate(DatacubeSelection* datacubeselection) :
    q(datacubeselection),
    datacube(0L),
    synchronized_selection_model(0L),
    ignore_synchronized(false),
    nrows(0),
    ncolumns(0)
{

}

void DatacubeSelectionPrivate::dump() {
  for (int r = 0; r < nrows; ++r) {
    for (int c = 0; c < ncolumns; ++c) {
      std::cout << std::setw(3) << cellValue(r, c) << " ";
    }
    std::cout << "\n";
  }
  std::cout << std::endl;
}


void DatacubeSelection::addElements(QList< int > elements) {
  QList<int> actually_selected_elements;
  Cell cell;
  Q_FOREACH(int element, elements) {
    if (!d->selected_elements.contains(element)) {
      d->datacube->d->bucket_for_element(element, cell);
      d->selected_elements << element;
      actually_selected_elements << element;
      if (!cell.invalid()) {
        int newvalue = d->increaseCell(cell.row(), cell.column(),1);
        if (newvalue == 1 || newvalue == d->datacube->d->elements_in_bucket(cell.row(), cell.column()).size()) {
          const int row_section = d->datacube->d->section_for_bucket_row(cell.row());
          const int column_section = d->datacube->d->section_for_bucket_column(cell.column());
          emit selectionStatusChanged(row_section,column_section);
        }
      }
    }
  }
  d->select_on_synchronized(actually_selected_elements);
}

void DatacubeSelection::removeElements(QList< int > elements) {
  QList<int> actually_deselected_elements;
  Cell cell;
  Q_FOREACH(int element, elements) {
    if (d->selected_elements.remove(element)) {
      d->datacube->d->bucket_for_element(element, cell);
      actually_deselected_elements << element;
      if (!cell.invalid()) {
        int newvalue = d->decreaseCell(cell.row(), cell.column());
        Q_ASSERT(newvalue>=0);
        if (newvalue == 0 || newvalue == d->datacube->d->elements_in_bucket(cell.row(), cell.column()).size()-1) {
          const int row_section = d->datacube->d->section_for_bucket_row(cell.row());
          const int column_section = d->datacube->d->section_for_bucket_column(cell.column());
          emit selectionStatusChanged(row_section,column_section);
        }
      }
    }
  }
  d->deselect_on_synchronized(actually_deselected_elements);

}


DatacubeSelection::DatacubeSelection (qdatacube::Datacube* datacube, qdatacube::DatacubeView* view) :
    QObject(view),
    d(new DatacubeSelectionPrivate(this)) {
  d->datacube = datacube;
  datacube->d->add_selection_model(this);
  connect(d->datacube, SIGNAL(reset()),d.data(), SLOT(reset()));
  d->reset();
}

void DatacubeSelectionPrivate::reset() {
    nrows = datacube->d->number_of_buckets(Qt::Vertical);
    ncolumns = datacube->d->number_of_buckets(Qt::Horizontal);
    QList<int> old_selected_elements = selected_elements.toList();
    cells.clear();
    selected_elements.clear();
    q->addElements(old_selected_elements);
}

void DatacubeSelection::addCell(int row, int column) {
  int bucket_row = d->datacube->d->bucket_for_row(row);
  int bucket_column = d->datacube->d->bucket_for_column(column);
  QList<int> raw_elements = d->datacube->d->elements_in_bucket(bucket_row, bucket_column);
  QList<int> elements;
  Q_FOREACH(int raw_element, raw_elements) {
    if (!d->selected_elements.contains(raw_element)) {
      elements << raw_element;
    }
  }
  if (!elements.isEmpty()) {
      d->increaseCell(bucket_row, bucket_column, elements.size());
    Q_FOREACH(int element, elements) {
      d->selected_elements << element;
    }
    emit selectionStatusChanged(row, column);
    d->select_on_synchronized(elements);
  }
}

void DatacubeSelectionPrivate::datacube_adds_element_to_bucket(int row, int column, int element) {
  if (selected_elements.contains(element)) {
    int c = increaseCell(row, column);
    Q_ASSERT(c <= datacube->d->elements_in_bucket(row, column).size()); Q_UNUSED(c);
  }
}

void DatacubeSelectionPrivate::datacube_removes_element_from_bucket(int row, int column, int element) {
  if (selected_elements.contains(element)) {
    int c = decreaseCell(row, column);
    Q_ASSERT(c >= 0); Q_UNUSED(c);
  }
}

DatacubeSelection::SelectionStatus DatacubeSelection::selectionStatus(int row, int column) const {
  const int bucket_row = d->datacube->d->bucket_for_row(row);
  const int bucket_column = d->datacube->d->bucket_for_column(column);
  const int selected_count = d->cellValue(bucket_row, bucket_column);
  if (selected_count > 0) {
    const int count = d->datacube->d->elements_in_bucket(bucket_row, bucket_column).size();
    if (selected_count == count) {
      return SELECTED;
    } else {
      return PARTIALLY_SELECTED;
    }
  } else {
    return UNSELECTED;
  }

}

void DatacubeSelection::clear() {
  d->selected_elements.clear();
  std::fill(d->cells.begin(), d->cells.end(), 0);
  d->clear_synchronized();
}

void DatacubeSelection::updateSelection(QItemSelection select, QItemSelection deselect) {
  if (!d->ignore_synchronized) {
    addElements(d->elements_from_selection(select));
    removeElements(d->elements_from_selection(deselect));
  }
}

DatacubeSelection::~DatacubeSelection() {
  // declared to have secret_t in scope
}

void DatacubeSelection::synchronizeWith(QItemSelectionModel* synchronized_selection_model) {
  if (d->synchronized_selection_model) {
    d->synchronized_selection_model->disconnect(SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateSelection(QItemSelection,QItemSelection)));
    d->synchronized_selection_model = 0L;
  }
  if (synchronized_selection_model) {
    d->synchronized_selection_model = synchronized_selection_model;
    connect(synchronized_selection_model,
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SLOT(updateSelection(QItemSelection,QItemSelection)));
    updateSelection(synchronized_selection_model->selection(), QItemSelection());
  }

}

void DatacubeSelectionPrivate::datacube_deletes_elements(int start, int end)
{
  const int adjust = (end-start)+1;
  QSet<int> new_selected_elements;
  Q_FOREACH(int e, selected_elements) {
    Q_ASSERT(e<start || e>end);
    new_selected_elements << (e>start? e-adjust:e);
  }
  selected_elements = new_selected_elements;

}

void DatacubeSelectionPrivate::datacube_inserts_elements(int start, int end) {
  const int adjust = (end-start)+1;
  QSet<int> new_selected_elements;
  Q_FOREACH(int e, selected_elements) {
    new_selected_elements << (e>=start? e+adjust:e);
  }
  selected_elements = new_selected_elements;

}

} // end of namespace

#include "datacubeselection.moc"
