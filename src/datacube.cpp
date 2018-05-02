/*
 Author: Ange Optimization <esben@ange.dk>  (C) Ange Optimization ApS 2009

 Copyright: See COPYING file that comes with this distribution

*/


/**
 * Vocabulary:
 *
 * In several functions, normal_count, normal_counts and similar constructs are used.
 * Normal in this context means 'surface normal', or 'the other direction' than what the function is working on.
 * parallel_* is used in the meaning 'the same direction'.
 *
 * Stride is used as the (final) number of buckets created in this direction with the given aggregator.
 */

// Enable this to get a lot of consistency checks
// #define ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS 1

#include "datacube.h"
#include "abstractaggregator.h"
#include "abstractfilter.h"

#include <QVector>
#include <algorithm>

#include <QAbstractItemModel>
#include "cell.h"
#include "datacubeselection.h"
#include "datacubeselection_p.h"

#include "datacube_p.h"

#include <QSharedPointer>

namespace qdatacube {

int DatacubePrivate::computeBucketForIndex(Qt::Orientation orientation, int index) {
  qdatacube::Datacube::Aggregators& aggregators = orientation == Qt::Horizontal ? col_aggregators : row_aggregators;
  int stride = 1;
  int rv = 0;
  for (int aggregator_index = aggregators.size()-1; aggregator_index>=0; --aggregator_index) {
    AbstractAggregator::Ptr aggregator = aggregators.at(aggregator_index);
    rv += stride * (*aggregator)(index);
    stride *= aggregator->categoryCount();
  }
  Q_ASSERT(rv >=0);
  return rv;
}

int DatacubePrivate::bucket_for_row(const int row) const {
  int r = row;
  for (int bucket = 0; bucket < row_counts.size(); ++bucket) {
    if (row_counts[bucket] > 0) {
      if (r-- == 0) {
        return bucket;
      }
    }
  }
  Q_ASSERT_X(false, "QDatacube", QString("Row %1 too big for qdatacube with %2 rows").arg(row).arg(row_counts.size() - std::count(row_counts.begin(), row_counts.end(), 0u)).toLocal8Bit().data());
  return -1;
}

int DatacubePrivate::bucket_for_column(int column) const {
  int c = column;
  for (int bucket = 0; bucket < col_counts.size(); ++bucket) {
    if (col_counts[bucket] > 0) {
      if (c-- == 0) {
        return bucket;
      }
    }
  }
  Q_ASSERT_X(false, "qdatacube", QString("Column %1 too big for qdatacube with %2 columns").arg(column).arg(col_counts.size() - std::count(col_counts.begin(), col_counts.end(), 0u)).toLocal8Bit().data());
  return -1;
}

const QList< int >& DatacubePrivate::cell(long bucket_row, long bucket_column) const {
  const long i = bucket_row + bucket_column*row_counts.size();
  cells_t::const_iterator it = cells.constFind(i);
  static const QList<int> empty_list;
  if(it == cells.constEnd()) {
    return empty_list;
  }
  return it.value();
}

void DatacubePrivate::cellAppend(CellPoint point, QList< int > listadd) {
    const long i = point.row + point.column*row_counts.size();
    cells[i].append(listadd);
}

void DatacubePrivate::cellAppend(long int bucket_row, long bucket_column, int to_add) {
    const long i = bucket_row + bucket_column*row_counts.size();
    Q_ASSERT(!cells[i].contains(to_add));
    cells[i].append(to_add);
}

bool DatacubePrivate::cellRemoveOne(long row, long column, int index) {
    const long i = row + column*row_counts.size();
    cells_t::iterator it = cells.find(i);
    if(it == cells.end()) {
        return false;
    }
    bool success = it.value().removeOne(index);
    if(it.value().isEmpty()) {
        cells.remove(i);
    }
    return success;
}

int DatacubePrivate::hasCell(long int bucket_row, long bucket_column) const {
    const long i = bucket_row + bucket_column*row_counts.size();
    cells_t::const_iterator it = cells.constFind(i);
    return it != cells.constEnd();
}

void DatacubePrivate::setCell(long int bucket_row, long bucket_column, const QList< int >& cell_content) {
    setCell(CellPoint(bucket_row, bucket_column), cell_content);
}

void DatacubePrivate::setCell(CellPoint point, const QList< int >& cell_content) {
    const long i = point.row + point.column*row_counts.size();
    cells_t::iterator it = cells.find(i);
    if(it == cells.end()) {
        if(!cell_content.isEmpty()) {
            cells.insert(i,cell_content);
        }
    } else {
        if(cell_content.isEmpty()) {
            cells.erase(it);
        } else {
            *it = cell_content;
        }
    }
}



int DatacubePrivate::bucket_to_column(int bucket_column) const {
  int rv = 0;
  for (int i=0; i<bucket_column; ++i) {
    if (col_counts[i]>0) {
      ++rv;
    }
  }
  return rv;

}

int DatacubePrivate::bucket_to_row(int bucket_row) const {
  int rv = 0;
  for (int i=0; i<bucket_row; ++i) {
    if (row_counts[i]>0) {
      ++rv;
    }
  }
  return rv;

}
DatacubePrivate::DatacubePrivate(Datacube* datacube, const QAbstractItemModel* model) :
                               q(datacube),
                               model(model)
{
  col_counts = QVector<unsigned>(1);
  row_counts = QVector<unsigned>(1);
}

DatacubePrivate::DatacubePrivate(Datacube* datacube, const QAbstractItemModel* model,
                               AbstractAggregator::Ptr row_aggregator,
                               AbstractAggregator::Ptr column_aggregator) :
    q(datacube),
    model(model)
{
  col_aggregators << column_aggregator;
  row_aggregators << row_aggregator;
  col_counts = QVector<unsigned>(column_aggregator->categoryCount());
  row_counts = QVector<unsigned>(row_aggregator->categoryCount());
}

Datacube::Datacube(const QAbstractItemModel* model,
                       AbstractAggregator::Ptr row_aggregator,
                       AbstractAggregator::Ptr column_aggregator,
                       QObject* parent):
    QObject(parent),
    d(new DatacubePrivate(this, model, row_aggregator, column_aggregator))
{
  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), d.data(), SLOT(update_data(QModelIndex,QModelIndex)));
  connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), d.data(), SLOT(remove_data(QModelIndex,int,int)));
  connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), d.data(), SLOT(insert_data(QModelIndex,int,int)));
  connect(column_aggregator.data(), SIGNAL(categoryAdded(int)), d.data(), SLOT(slot_aggregator_category_added(int)));
  connect(row_aggregator.data(), SIGNAL(categoryAdded(int)), d.data(), SLOT(slot_aggregator_category_added(int)));
  connect(column_aggregator.data(), SIGNAL(categoryRemoved(int)), d.data(), SLOT(slot_aggregator_category_removed(int)));
  connect(row_aggregator.data(), SIGNAL(categoryRemoved(int)), d.data(), SLOT(slot_aggregator_category_removed(int)));;
  for (int element = 0, nelements = model->rowCount(); element < nelements; ++element) {
    d->add(element);
  }
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif

}

Datacube::Datacube(const QAbstractItemModel* model, QObject* parent)
  : QObject(parent),
    d(new DatacubePrivate(this, model))
{
  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), d.data(), SLOT(update_data(QModelIndex,QModelIndex)));
  connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), d.data(), SLOT(remove_data(QModelIndex,int,int)));
  connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), d.data(), SLOT(insert_data(QModelIndex,int,int)));
  for (int element = 0, nelements = model->rowCount(); element < nelements; ++element) {
    d->add(element);
  }
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif
}


void Datacube::addFilter(AbstractFilter::Ptr filter) {
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif
  if (!filter) {
    return;
  }
  for (int row = 0, nrows = d->model->rowCount(); row<nrows; ++row) {
    const bool was_included = d->reverse_index.contains(row);
    const bool included = (*filter)(row);
    if (was_included && !included) {
      d->remove(row);
    }
  }
  d->filters << filter;
  emit filterChanged();
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif
}

bool Datacube::removeFilter(AbstractFilter::Ptr filter)
{
  for (Filters::iterator it = d->filters.begin(), iend = d->filters.end(); it != iend; ++it) {
    if (*it == filter) {
      Filters::value_type removed_filter = *it;
      d->filters.erase(it);
      for (int row = 0, nrows = d->model->rowCount(); row<nrows; ++row) {
        if (d->filtered_in(row)) {
          const bool excluded = !(*filter)(row);
          if (excluded) {
            d->add(row);
          }
        }
      }
      emit filterChanged();
      return true;
    }
  }
  return false;
}


void Datacube::check() const {
  int total_count = 0;
  const int nelements = d->model->rowCount();
  for (int i=0; i<nelements;  ++i) {
    bool included = true;
    for (int filter_index=0, last_filter_index = d->filters.size()-1; filter_index<=last_filter_index; ++filter_index) {
      Filters::const_reference filter = d->filters.at(filter_index);
      if (!(*filter)(i)) {
        included = false;
        break;
      }
    }
    if (included) {
      ++total_count;
    }
  }
  int failcols = 0;
  int failrows = 0;
  QVector<unsigned> check_row_counts(d->row_counts.size());
  int count = 0;
  for (int c=0; c<d->col_counts.size(); ++c) {
    unsigned col_count = 0;
    for (int r=0; r<d->row_counts.size(); ++r) {
      const int nelements = d->cell(r,c).size();
      check_row_counts[r] += nelements;
      col_count += nelements;
    }
    if (col_count != d->col_counts[c]) {
      qDebug() << "col" << "found, expected" << col_count << "!=" << d->col_counts[c];
      failcols++;
      Q_ASSERT(col_count == d->col_counts[c]);
    }
    count += col_count;
  }
  Q_ASSERT_X(count == total_count, __func__, QString("%1 == %2").arg(count).arg(total_count).toLocal8Bit().data());
  for (int i=0; i<d->row_counts.size(); ++i) {
        if(check_row_counts[i] != d->row_counts[i]) {
            qDebug() << "row" << "found, expected" << check_row_counts[i] << "!=" << d->row_counts[i];
            failrows++;
            Q_ASSERT(check_row_counts[i] == d->row_counts[i]);
        }
  }
    qDebug() << "check done" << failcols << failrows;
}

void Datacube::resetFilter() {
  if (d->filters.empty()) {
    return;
  }
  d->filters.clear();
  for (int row = 0, nrows = d->model->rowCount(); row<nrows; ++row) {
    const bool was_included = d->reverse_index.contains(row);
    if (!was_included) {
      d->add(row);
    }
  }
  emit filterChanged();

}

int Datacube::headerCount(Qt::Orientation orientation) const {
  return orientation == Qt::Horizontal ? d->col_aggregators.size() : d->row_aggregators.size();
}

int Datacube::elementCount(int row, int column) const {
  return  elements(row,column).size();
}

int Datacube::columnCount() const {
  return d->col_counts.size() - std::count(d->col_counts.begin(), d->col_counts.end(), 0u);
}

int Datacube::rowCount() const {
  return d->row_counts.size() - std::count(d->row_counts.begin(), d->row_counts.end(), 0u);
}

QList< int > Datacube::elements(int row, int column) const {
  // Note that this function should be very fast indeed.
  const int row_section = d->bucket_for_row(row);
  const int col_section = d->bucket_for_column(column);
  return d->cell(row_section, col_section);

}

QList< Datacube::HeaderDescription > Datacube::headers(Qt::Orientation orientation, int index) const {
  QList< HeaderDescription > rv;
  Aggregators& aggregators = (orientation == Qt::Horizontal) ? d->col_aggregators : d->row_aggregators;
  const QVector<unsigned>& counts = (orientation == Qt::Horizontal) ? d->col_counts : d->row_counts;
  AbstractAggregator::Ptr aggregator = aggregators.at(index);
  const int ncats = aggregator->categoryCount();
  int stride = 1;
  for (int i=index+1; i<aggregators.size(); ++i) {
    stride *= aggregators.at(i)->categoryCount();
  }
  for (int c=0; c<counts.size(); c+=stride) {
    int count = 0;
    for (int i=0;i<stride; ++i) {
      if (counts.at(c+i)>0) {
        ++count;
      }
    }
    if (count > 0 ) {
      rv << HeaderDescription((c/stride)%ncats, count);
    }
  }
  return rv;
}

Datacube::~Datacube() {
  // Need to declare here so datacube_colrow_t's destructor is visible
}

void DatacubePrivate::add(int index) {
    Q_ASSERT(index < model->rowCount());

  // Compute bucket
  int rowBucket = computeRowBucketForIndex(index);
  if (rowBucket == -1) {
    // Our datacube does not cover that container. Just ignore it.
    return;
  }
  int columnBucket = computeColumnBucketForIndex(index);
  Q_ASSERT(columnBucket>=0); // Every container should be in both rows and columns, or neither place.

  // Check if rows/columns are added, and notify listernes as neccessary
  int row_to_add = -1;
  int column_to_add = -1;
    {
        unsigned int& section_count = row_counts[rowBucket];
        section_count += 1;
        if(section_count == 1) {
            row_to_add = bucket_to_row(rowBucket);;
            emit q->rowsAboutToBeInserted(row_to_add,1);
        }
    }
    {
        unsigned int& section_count = col_counts[columnBucket];
        section_count += 1;
        if(section_count == 1) {
            column_to_add = bucket_to_column(columnBucket);
            emit q->columnsAboutToBeInserted(column_to_add,1);
        }
    }

  // Actually add
  cellAppend(rowBucket, columnBucket,index);
  Q_ASSERT(!reverse_index.contains(index));
  reverse_index.insert(index, Cell(rowBucket, columnBucket));

  // Notify various listerners
  Q_FOREACH(DatacubeSelection* selection, selection_models) {
    selection->d->datacube_adds_element_to_bucket(rowBucket, columnBucket, index);
  }
  if(column_to_add>=0) {
    emit q->columnsInserted(column_to_add,1);
  }
  if(row_to_add>=0) {
    emit q->rowsInserted(row_to_add,1);
  }
  if(row_to_add==-1 && column_to_add==-1) {
    emit q->dataChanged(bucket_to_row(rowBucket),bucket_to_column(columnBucket));
  }
}

void DatacubePrivate::remove(int index) {
  Cell cell = reverse_index.value(index);
  if (cell.invalid()) {
    // Our datacube does not cover that container. Just ignore it.
    return;
  }
  Q_FOREACH(DatacubeSelection* selection, selection_models) {
    selection->d->datacube_removes_element_from_bucket(cell.row(), cell.column(), index);
  }
  int row_to_remove = -1;
  int column_to_remove = -1;
  if(--row_counts[cell.row()]==0) {
    row_to_remove = bucket_to_row(cell.row());
    emit q->rowsAboutToBeRemoved(row_to_remove,1);
  }
  if(--col_counts[cell.column()]==0) {
    column_to_remove = bucket_to_column(cell.column());
    emit q->columnsAboutToBeRemoved(column_to_remove,1);
  }
  Q_ASSERT(hasCell(cell.row(),cell.column()));
  const bool check = cellRemoveOne(cell.row(), cell.column(),index);
  Q_UNUSED(check)
  Q_ASSERT(check);
  reverse_index.remove(index);
  if(column_to_remove>=0) {
    emit q->columnsRemoved(column_to_remove,1);
  }
  if(row_to_remove>=0) {
    emit q->rowsRemoved(row_to_remove,1);
  }
  if(row_to_remove==-1 && column_to_remove==-1) {
    emit q->dataChanged(bucket_to_row(cell.row()),bucket_to_column(cell.column()));
  }
}

void DatacubePrivate::update_data(QModelIndex topleft, QModelIndex bottomRight) {
  const int toprow = topleft.row();
  const int buttomrow = bottomRight.row();
  for (int element = toprow; element <= buttomrow; ++element) {
    const bool filtered_out = !filtered_in(element);
    int new_row_section = computeBucketForIndex(Qt::Vertical, element);
    int new_column_section = computeBucketForIndex(Qt::Horizontal, element);
    Cell old_cell = reverse_index.value(element);
    const bool rowchanged = old_cell.row() != new_row_section;
    const bool colchanged = old_cell.column() != new_column_section;
    if (rowchanged || colchanged || filtered_out) {
      remove(element);
      if (!filtered_out) {
        add(element);
      }
    }
  }
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif
}

void DatacubePrivate::insert_data(QModelIndex parent, int start, int end) {
  Q_ASSERT(!parent.isValid());
  Q_UNUSED(parent);
  Q_FOREACH(DatacubeSelection* selection, selection_models) {
    selection->d->datacube_inserts_elements(start, end);
  }
  renumber_cells(start, end-start+1);
  for (int row = start; row <=end; ++row) {
    if(filtered_in(row)) {
      add(row);
    }
  }
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif

}

void DatacubePrivate::remove_data(QModelIndex parent, int start, int end) {
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif
  Q_ASSERT(!parent.isValid());
  Q_UNUSED(parent);
  for (int row = end; row>=start; --row) {
    remove(row);
  }
  Q_FOREACH(DatacubeSelection* selection, selection_models) {
    selection->d->datacube_deletes_elements(start, end);
  }
  // Now, all the remaining elements have to be renumbered
  renumber_cells(end+1, start-end-1);

}

void DatacubePrivate::renumber_cells(int start, int adjustment) {
  reverse_index_t new_index;
  for (cells_t::iterator it = cells.begin(), iend = cells.end(); it != iend; ++it) {
    for (QList<int>::iterator jit = it->begin(), jend = it->end(); jit != jend; ++jit) {
      Cell cell = reverse_index.value(*jit);
      if (*jit >= start) {
        *jit += adjustment;
      }
      new_index.insert(*jit, cell);
    }
  }
  reverse_index = new_index;

}

void DatacubePrivate::slot_columns_changed(int column, int count) {
  for (int col = column; col<=column+count;++col) {
    const int rowcount = q->rowCount();
    for (int row = 0; row < rowcount; ++row) {
      emit q->dataChanged(row,col);
    }
  }
  emit q->headersChanged(Qt::Horizontal, column, column+count-1);
}

void DatacubePrivate::slot_rows_changed(int row, int count) {
  for (int r = row; r<=row+count;++r) {
    const int columncount = q->columnCount();
    for (int column = 0; column < columncount; ++column) {
      emit q->dataChanged(r,column);
    }
  }
  emit q->headersChanged(Qt::Vertical, row, row+count-1);
}

void Datacube::split(Qt::Orientation orientation, int headerno, AbstractAggregator::Ptr aggregator) {
  emit aboutToBeReset();
  if (orientation == Qt::Vertical) {
    d->split_row(headerno, aggregator);
  } else {
    d->split_column(headerno, aggregator);
  }
  connect(aggregator.data(), SIGNAL(categoryAdded(int)), d.data(), SLOT(slot_aggregator_category_added(int)));
  connect(aggregator.data(), SIGNAL(categoryRemoved(int)), d.data(), SLOT(slot_aggregator_category_removed(int)));;
  emit reset();
}

void DatacubePrivate::split_row(int headerno, AbstractAggregator::Ptr aggregator)
{
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif
  const int ncats = aggregator->categoryCount();
  const int source_row_count = row_counts.size();
  const long target_row_countl = long(source_row_count) * long(ncats);
  if(INT_MAX/10 < target_row_countl) {
    qWarning("We are overflowing! Avoiding it by not splitting row.");
    return;
  }
  const int target_row_count = target_row_countl;
  emit q->aboutToBeReset();
  DatacubePrivate::cells_t oldcells = cells;
  cells = DatacubePrivate::cells_t();
  int cat_stride = 1;
  for (int i=headerno; i< row_aggregators.size(); ++i) {
    cat_stride *= row_aggregators.at(i)->categoryCount();
  }
  int target_stride = cat_stride*ncats;
  QVector<unsigned> old_row_counts = row_counts;
  row_counts = QVector<unsigned>(old_row_counts.size()*ncats);
  reverse_index = DatacubePrivate::reverse_index_t();
  // Sort out elements in new categories. Note that the old d->col_counts are unchanged
  for(DatacubePrivate::cells_t::const_iterator it = oldcells.constBegin(), end = oldcells.constEnd(); it!= end; ++it) {
        const long index = it.key();
        const long c = index / source_row_count;
        const long r = index % source_row_count;
        const long major = r / cat_stride;
        const long minor = r % cat_stride;
        Q_FOREACH(int element,it.value()) {
            const long target_row = major*target_stride + minor + (*aggregator).operator()(element) * cat_stride;
            const long target_index = target_row + long(c)*target_row_count;
            cells[target_index] << element;
            ++row_counts[target_row];
            reverse_index.insert(element, Cell(target_row, c));
        }
  }
  row_aggregators.insert(headerno, aggregator);
  emit q->reset();
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif

}

void DatacubePrivate::split_column(int headerno, AbstractAggregator::Ptr aggregator) {
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif
  const int ncats = aggregator->categoryCount();
  const int old_column_count = col_counts.size();
  const long new_column_countl = long(old_column_count) * long(ncats);
  if(INT_MAX/10 < new_column_countl) {
    qWarning("We are overflowing! Avoiding it by not splitting column.");
    return;
  }
  emit q->aboutToBeReset();
  DatacubePrivate::cells_t oldcells = cells;
  cells = DatacubePrivate::cells_t();
  const int row_count = row_counts.size();
  int cat_stride = 1;
  for (int i=headerno; i< col_aggregators.size(); ++i) {
    cat_stride *= col_aggregators.at(i)->categoryCount();
  }
  int target_stride = cat_stride*ncats;
  QVector<unsigned> old_column_counts = col_counts;
  col_counts = QVector<unsigned>(int(new_column_countl));
  reverse_index = DatacubePrivate::reverse_index_t();
  // Sort out elements in new categories. Note that the old d->row_counts are unchanged

  for(DatacubePrivate::cells_t::const_iterator it = oldcells.constBegin(), end = oldcells.constEnd(); it!= end; ++it) {
        const long index = it.key();
        const long r = index % row_count;
        const long c = index/row_count;
        const long major = c/cat_stride;
        const long minor = c%cat_stride;
        Q_FOREACH(int element,it.value()) {
            const long target_column = major*target_stride + minor + (*aggregator).operator()(element) * cat_stride;
            const long target_index = r + long(target_column)*row_counts.size();
            cells[target_index] << element;
            reverse_index.insert(element, Cell(r, target_column));
            ++col_counts[target_column];
        }
  }
  col_aggregators.insert(headerno, aggregator);
  emit q->reset();
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  q->check();
#endif

}

void Datacube::collapse(Qt::Orientation orientation, int headerno) {
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif
  emit aboutToBeReset();
  DatacubePrivate::cells_t oldcells = d->cells;
  const bool horizontal = (orientation == Qt::Horizontal);
  Datacube::Aggregators& parallel_aggregators = horizontal ? d->col_aggregators : d->row_aggregators;
  AbstractAggregator::Ptr aggregator = parallel_aggregators[headerno];
  disconnect(aggregator.data(), SIGNAL(categoryAdded(int)), d.data(), SLOT(slot_aggregator_category_added(int)));
  disconnect(aggregator.data(), SIGNAL(categoryRemoved(int)), d.data(), SLOT(slot_aggregator_category_removed(int)));;
  parallel_aggregators.removeAt(headerno);
  const int ncats = aggregator->categoryCount();
  d->cells = DatacubePrivate::cells_t();
  const int normal_count = horizontal ? d->row_counts.size() : d->col_counts.size();
  int cat_stride = 1;
  for (int i=headerno; i<parallel_aggregators.size(); ++i) {
    cat_stride *= parallel_aggregators.at(i)->categoryCount();
  }
  const int source_stride = cat_stride * ncats;
  QVector<unsigned>& new_counts = horizontal ? d->col_counts : d->row_counts;
  QVector<unsigned> old_counts = new_counts;
  new_counts = QVector<unsigned>(old_counts.size()/ncats);
  d->reverse_index = DatacubePrivate::reverse_index_t();
  for (int major=0; major<(new_counts.size()/cat_stride); ++major) {
    for (int minor=0; minor<cat_stride; ++minor) {
      const int p = major*cat_stride+minor;
      unsigned& count = new_counts[p];
      for (int n=0; n<normal_count; ++n) {
        QList<int> cell;
        for (int i = 0; i<ncats; ++i) {
          const int old_parallel_index = major*source_stride+minor+i*cat_stride;
          QList<int> oldcell = oldcells.value(horizontal ? (old_parallel_index)*normal_count+n : n*old_counts.size()+old_parallel_index);
          cell.append(oldcell);
          count += oldcell.size();
          Q_FOREACH(int element, oldcell) {
            d->reverse_index.insert(element, horizontal ? Cell(n,p) : Cell(p, n));
          }
        }
        d->cellAppend(horizontal ? CellPoint(n, p) : CellPoint(p, n),cell);
      }
    }
  }
  emit reset();
#ifdef ANGE_QDATACUBE_CHECK_PRE_POST_CONDITIONS
  check();
#endif
}

int Datacube::sectionForElement(int element, Qt::Orientation orientation) const {
  const int section = d->computeBucketForIndex(orientation, element);
  return orientation == Qt::Horizontal ? d->bucket_to_column(section) : d->bucket_to_row(section);
}

int Datacube::internalSection(int element, Qt::Orientation orientation) const {
  if (orientation == Qt::Horizontal) {
    return d->bucket_to_column(d->reverse_index.value(element).column());
  } else  {
    return d->bucket_to_row(d->reverse_index.value(element).row());
  }

}

void DatacubePrivate::bucket_for_element(int element, Cell& result) const {
  result = reverse_index.value(element);
}

Datacube::Filters Datacube::filters() const {
  return d->filters;
}

const QAbstractItemModel* qdatacube::Datacube::underlyingModel() const {
  return d->model;
}

} // namespace

void qdatacube::Datacube::dump(bool cells, bool rowcounts, bool col_counts) const {
  if (col_counts) {
    qDebug() << "col_counts: " << d->col_counts;
  }
  if (rowcounts) {
    qDebug() << "row_counts: " << d->row_counts;
  }
  if (cells) {
    qDebug() << "Check: " << d->row_counts.size() << " * " << d->col_counts.size() << "=" << d->cells.size();
  }
  for (int r=0; r<d->row_counts.size(); ++r) {
    QList<int> row;
    for (int c=0; c<d->col_counts.size(); ++c) {
      row << d->cells.value(r+d->row_counts.size()*c).size();
    }
    qDebug() << row;
  }

}

void qdatacube::DatacubePrivate::slot_aggregator_category_added(int newCategoryIndex) {
  if (AbstractAggregator* aggregator = qobject_cast<AbstractAggregator*>(sender())) {
    int headerno = 0;
    Q_FOREACH(AbstractAggregator::Ptr f, row_aggregators) {
      if (f == aggregator) {
        aggregator_category_added(f, headerno, newCategoryIndex, Qt::Vertical);
      }
      ++headerno;
    }
    headerno = 0;
    Q_FOREACH(AbstractAggregator::Ptr f, col_aggregators) {
      if (f == aggregator) {
        aggregator_category_added(f, headerno, newCategoryIndex, Qt::Horizontal);
      }
      ++headerno;
    }
  }
}

void qdatacube::DatacubePrivate::slot_aggregator_category_removed(int categoryIndex) {
  if (AbstractAggregator* aggregator = qobject_cast<AbstractAggregator*>(sender())) {
    int headerno = 0;
    Q_FOREACH(AbstractAggregator::Ptr f, row_aggregators) {
      if (f == aggregator) {
        aggregator_category_removed(f, headerno, categoryIndex, Qt::Vertical);
      }
      ++headerno;
    }
    headerno = 0;
    Q_FOREACH(AbstractAggregator::Ptr f, col_aggregators) {
      if (f == aggregator) {
        aggregator_category_removed(f, headerno, categoryIndex, Qt::Horizontal);
      }
      ++headerno;
    }
  }

}


void qdatacube::DatacubePrivate::aggregator_category_added(qdatacube::AbstractAggregator::Ptr aggregator, int headerno, int newCategoryIndex, Qt::Orientation orientation)
{
  const Datacube::Aggregators& parallel_aggregators = orientation == Qt::Horizontal ? col_aggregators : row_aggregators;
  QVector<unsigned>& new_parallel_counts = orientation == Qt::Horizontal ? col_counts : row_counts;
  const QVector<unsigned>& normal_counts = orientation == Qt::Horizontal ? row_counts : col_counts;
  const int normal_count = normal_counts.size();
  const QVector<unsigned> old_parallel_counts = new_parallel_counts;
  DatacubePrivate::cells_t old_cells = cells;
  int nsuper_categories = 1;
  for (int h=0; h<headerno; ++h) {
    nsuper_categories *= qMax(parallel_aggregators[h]->categoryCount(),1);
  }
  const int new_ncats = aggregator->categoryCount();
  int n_new_parallel_counts = nsuper_categories * new_ncats;
  for (int h = headerno+1; h<parallel_aggregators.size(); ++h) {
    n_new_parallel_counts *= parallel_aggregators[h]->categoryCount();
  }
  new_parallel_counts = QVector<unsigned>(n_new_parallel_counts);
  cells.clear();
#if !QT_NO_DEBUG
  int debug_reverseIndexSize = reverse_index.size();
#endif
  if (!reverse_index.empty()) { // If there is no elements in the recap, it is possible some of the aggregators have no categories.
    const int old_ncats = new_ncats - 1;
    const int stride = old_parallel_counts.size()/old_ncats/nsuper_categories;
    reverse_index.clear(); // TODO This should not be needed as we rewrite the entire reverse index
    for (int normal_index=0; normal_index<normal_count; ++normal_index) {
        if (normal_counts[normal_index] == 0) {
            continue;
        }
      for (int super_index=0; super_index<nsuper_categories; ++super_index) {
        for (int category_index=0; category_index<old_ncats; ++category_index) {
          for (int sub_index=0; sub_index<stride; ++sub_index) {
            const int old_p = super_index*stride*old_ncats + category_index*stride + sub_index;
            if(old_parallel_counts[old_p] == 0) {
                continue;
            }
            const int new_category_index = newCategoryIndex <=category_index ? category_index+1 : category_index;
            const int p = super_index*stride*new_ncats + new_category_index*stride + sub_index;
            new_parallel_counts[p] = old_parallel_counts[old_p];
            const QList<int>& values = orientation == Qt::Horizontal ? old_cells.value(normal_index+ old_p*normal_count) : old_cells.value(normal_index*old_parallel_counts.size()+old_p);
            if(!values.isEmpty()) {
                setCell(orientation == Qt::Horizontal ? CellPoint(normal_index, p) : CellPoint(p, normal_index),
                    values);
                Q_FOREACH(int element,  values) {
                    reverse_index.insert(element, orientation == Qt::Horizontal ? Cell(normal_index,p) : Cell(p, normal_index));
                }
            }
          }
        }
      }
    }
  }
  Q_ASSERT(debug_reverseIndexSize == reverse_index.size());
  emit q->reset(); // TODO: It is not impossible to emit the correct row/column changed instead
  // we can't do a check here because a element might be added to the model and about to be registered in the datacube
}

void qdatacube::DatacubePrivate::aggregator_category_removed(qdatacube::AbstractAggregator::Ptr aggregator, int headerno, int index, Qt::Orientation orientation)
{
  const Datacube::Aggregators& parallel_aggregators = orientation == Qt::Horizontal ? col_aggregators : row_aggregators;
  QVector<unsigned>& new_parallel_counts = orientation == Qt::Horizontal ? col_counts : row_counts;
  const int normal_count = orientation == Qt::Horizontal ? row_counts.size() : col_counts.size();
  const QVector<unsigned>& normal_counts = orientation == Qt::Horizontal ? row_counts : col_counts;
  const QVector<unsigned> old_parallel_counts = new_parallel_counts;
  DatacubePrivate::cells_t old_cells = cells;
  int nsuper_categories = 1;
  for (int h=0; h<headerno; ++h) {
    nsuper_categories *= qMax(parallel_aggregators[h]->categoryCount(),1);
  }
  const int new_ncats = aggregator->categoryCount();
  const int old_ncats = new_ncats + 1;
  const int stride = old_parallel_counts.size()/old_ncats/nsuper_categories;
  Q_ASSERT(stride*nsuper_categories*old_ncats == old_parallel_counts.size());
  new_parallel_counts = QVector<unsigned>(new_ncats * nsuper_categories * stride);
  cells = DatacubePrivate::cells_t();
  reverse_index.clear();
  for (int normal_index=0; normal_index<normal_count; ++normal_index) {
    if(normal_counts[normal_index] == 0) {
        continue;
    }
    for (int super_index=0; super_index<nsuper_categories; ++super_index) {
      for (int category_index=0; category_index<new_ncats; ++category_index) {
        for (int sub_index=0; sub_index<stride; ++sub_index) {
          const int old_category_index = index<=category_index ? category_index+1 : category_index;
          const int old_p = super_index*stride*old_ncats + old_category_index*stride + sub_index;
          if (old_parallel_counts[old_p] == 0) {
              continue;
          }
          const int p = super_index*stride*new_ncats + category_index*stride + sub_index;
          const QList<int>& value = orientation == Qt::Horizontal ? old_cells.value(normal_index+ old_p*normal_count) : old_cells.value(normal_index*old_parallel_counts.size()+old_p);
          new_parallel_counts[p] = old_parallel_counts[old_p];
          if(!value.isEmpty()) {
            setCell(orientation == Qt::Horizontal ? CellPoint(normal_index, p) : CellPoint(p, normal_index), value);
            Q_FOREACH(int element, value) {
                reverse_index.insert(element, orientation == Qt::Horizontal ? Cell(normal_index,p) : Cell(p, normal_index));
            }
          }
        }
      }
    }
  }
  emit q->reset(); // TODO: It is not impossible to emit the correct row/column changed instead
  // we can't do a check here because a element might be added to the model and about to be registered in the datacube
}

QList<int> qdatacube::DatacubePrivate::elements_in_bucket(int row, int column) const {
  return cell(row, column);

}

int qdatacube::DatacubePrivate::number_of_buckets(Qt::Orientation orientation) const {
  return orientation == Qt::Vertical ? row_counts.size() : col_counts.size();
}

void qdatacube::DatacubePrivate::add_selection_model(qdatacube::DatacubeSelection* selection) {
  selection_models << selection;
  connect(selection, SIGNAL(destroyed(QObject*)), SLOT(remove_selection_model(QObject*)));
}

int qdatacube::DatacubePrivate::section_for_bucket_column(int bucket_column) const {
  return bucket_to_column(bucket_column);
}

int qdatacube::DatacubePrivate::section_for_bucket_row(int bucket_row) const {
  return bucket_to_row(bucket_row);
}

void qdatacube::DatacubePrivate::remove_selection_model(QObject* selection_model) {
  selection_models.removeAll(static_cast<DatacubeSelection*>(selection_model));
}

int qdatacube::Datacube::categoryIndex(Qt::Orientation orientation, int header_index, int section) const {
  const int bucket = (orientation == Qt::Vertical) ? d->bucket_for_row(section) : d->bucket_for_column(section);
  int sub_header_size = 1;
  const Datacube::Aggregators& aggregators = (orientation == Qt::Vertical) ? d->row_aggregators : d->col_aggregators;
  for (int i=header_index+1; i<aggregators.size(); ++i) {
    sub_header_size *= aggregators[i]->categoryCount();
  }
  const int naggregator_categories = aggregators[header_index]->categoryCount();
  return bucket % (naggregator_categories*sub_header_size)/sub_header_size;
}

bool qdatacube::DatacubePrivate::filtered_in(int element) const {
  Q_FOREACH(Datacube::Filters::value_type filter, filters) {
    if (!(*filter)(element)) {
      return false;
    }
  }
  return true;
}

qdatacube::Datacube::Aggregators qdatacube::Datacube::columnAggregators() const
{
  return d->col_aggregators;
}

qdatacube::Datacube::Aggregators qdatacube::Datacube::rowAggregators() const
{
  return d->row_aggregators;
}

int qdatacube::Datacube::elementCount(Qt::Orientation orientation, int headerno, int header_section) const
{
  Aggregators& aggregators = (orientation == Qt::Horizontal) ? d->col_aggregators : d->row_aggregators;
  const QVector<unsigned>& counts = (orientation == Qt::Horizontal) ? d->col_counts : d->row_counts;
  int count = 0;
  int stride = 1;
  for (int i=headerno+1; i<aggregators.size(); ++i) {
    stride *= aggregators.at(i)->categoryCount();
  }
  int offset = 0;
  for (int section_index = 0; section_index <= header_section; section_index += (count>0) ? 1 : 0) {
    Q_ASSERT_X(offset < counts.size(), "QDatacube", QString("Section %1 at header %2 orientation %3 too big for qdatacube").arg(header_section).arg(headerno).arg(orientation == Qt::Horizontal ? "Horizontal" : "Vertical").toLocal8Bit().data());
    count = 0;
    for (int i=0; i<stride; ++i) {
      count += counts.at(offset+i);
    }
    offset += stride;
  }
  return count;
}

QList<int> qdatacube::Datacube::elements(Qt::Orientation orientation, int headerno, int header_section) const
{
  Aggregators& aggregators = (orientation == Qt::Horizontal) ? d->col_aggregators : d->row_aggregators;
  const QVector<unsigned>& counts = (orientation == Qt::Horizontal) ? d->col_counts : d->row_counts;
  int stride = 1;
  for (int i=headerno+1; i<aggregators.size(); ++i) {
    stride *= aggregators.at(i)->categoryCount();
  }
  // Skip forward to section
  int bucket = 0;
  int section = 0;
  for (int hs=0; bucket<counts.size() && hs < header_section; bucket+=stride) {
    int old_section = section;
    for (int i=0;i<stride; ++i) {
      if (counts.at(bucket+i)>0) {
        ++section;
      }
    }
    if (section>old_section) {
      ++hs;
    }
  }

  // Accumulate answer
  QList<int> rv;
  const int normal_count = (orientation == Qt::Horizontal) ? d->row_counts.size() : d->col_counts.size();
  for (; bucket<counts.size() && rv.isEmpty(); bucket+=stride) {
    for (int i=0;i<stride && (bucket+i)<counts.size(); ++i) {
      if (counts.at(bucket+i)>0) {
        for (int n=0; n<normal_count; ++n) {
          rv << ((orientation == Qt::Horizontal) ? d->cell(n,bucket+i) : d->cell(bucket+i,n));
        }
      }
    }
  }
  return rv;

}

int qdatacube::Datacube::toHeaderSection(const Qt::Orientation orientation, const int headerno, const int section) const
{
//   qDebug() << __func__ << headerno << section << row_count() << column_count();
  Aggregators& aggregators = (orientation == Qt::Horizontal) ? d->col_aggregators : d->row_aggregators;
  const QVector<unsigned>& counts = (orientation == Qt::Horizontal) ? d->col_counts : d->row_counts;
  int stride = 1;
  for (int i=headerno+1; i<aggregators.size(); ++i) {
    stride *= aggregators.at(i)->categoryCount();
  }
  // Skip forward to section
  int bucket = 0;
  int s = 0;
  int header_section = 0;
  for (; bucket<counts.size(); bucket+=stride) {
    int old_section = s;
    for (int i=0;i<stride; ++i) {
//       qDebug() << __func__ << bucket << s << header_section << stride << counts.at(bucket+i) << counts.size();
      if (counts.at(bucket+i)>0) {
        if (s++==section) {
          return header_section;
        }
      }
    }
    if (s>old_section) {
      ++header_section;
    }
  }
  Q_ASSERT_X(bucket<counts.size(), "QDatacube", QString("Section %1 in datacube orientation %3 too big for qdatacube").arg(section).arg(headerno).arg(orientation == Qt::Horizontal ? "Horizontal" : "Vertical").toLocal8Bit().data());
  return header_section;
}

QPair< int, int > qdatacube::Datacube::toSection(Qt::Orientation orientation, const int headerno, const int header_section) const
{
  Aggregators& aggregators = (orientation == Qt::Horizontal) ? d->col_aggregators : d->row_aggregators;
  const QVector<unsigned>& counts = (orientation == Qt::Horizontal) ? d->col_counts : d->row_counts;
  int stride = 1;
  for (int i=headerno+1; i<aggregators.size(); ++i) {
    stride *= aggregators.at(i)->categoryCount();
  }
  // Skip forward to section
  int bucket = 0;
  int section = 0;
  for (int hs=0; bucket<counts.size() && hs < header_section; bucket+=stride) {
    int old_section = section;
    for (int i=0;i<stride; ++i) {
      if (counts.at(bucket+i)>0) {
        ++section;
      }
    }
    if (section>old_section) {
      ++hs;
    }
  }

  // count number of (active) sections
  int count = 0;
  for (; bucket<counts.size() && count == 0; bucket+=stride) {
    for (int i=0;i<stride && (bucket+i)<counts.size(); ++i) {
      if (counts.at(bucket+i)>0) {
        ++count;
      }
    }
  }
  Q_ASSERT_X(count > 0, "QDatacube", QString("Section %1 in header %2 orientation %3 too big for qdatacube").arg(header_section).arg(headerno).arg(orientation == Qt::Horizontal ? "Horizontal" : "Vertical").toLocal8Bit().data());
  return QPair<int,int>(section,section+count-1);
}

int qdatacube::Datacube::elementCount() const
{
  return d->reverse_index.count();
}

QList< int > qdatacube::Datacube::elements() const
{
  return d->reverse_index.keys();
}

#include "datacube.moc"
