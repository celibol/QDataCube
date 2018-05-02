#ifndef QDATACUBE_DATACUBE_P_H
#define QDATACUBE_DATACUBE_P_H

#include <QObject>
#include <QSharedPointer>

#include "cell.h"
#include "datacube.h"

class QAbstractItemModel;
namespace qdatacube {
class DatacubeSelection;
}

namespace qdatacube {

struct CellPoint {
    long row;
    long column;
    CellPoint(int row, int column) : row(row), column(column) {}
};

class DatacubePrivate : public QObject {
    Q_OBJECT
    public:
        DatacubePrivate(Datacube* datacube, const QAbstractItemModel* model,
                AbstractAggregator::Ptr row_aggregator,
                AbstractAggregator::Ptr column_aggregator);
        DatacubePrivate(Datacube* datacube, const QAbstractItemModel* model);
        Datacube* q;
        int computeRowBucketForIndex(int index) {
            return computeBucketForIndex(Qt::Vertical, index);
        }
        int computeColumnBucketForIndex(int index) {
            return computeBucketForIndex(Qt::Horizontal, index);
        }
        int computeBucketForIndex(Qt::Orientation orientation, int index);
        const QList<int>& cell(long int bucket_row, long int bucket_column) const;
        int hasCell(long int bucket_row, long int bucket_column) const;
        void setCell(long int bucket_row, long int bucket_column, const QList< int >& cell_content);
        void setCell(qdatacube::CellPoint point, const QList< int >& cell_content);
        void cellAppend(long int bucket_row, long int bucket_column, int to_add);
        void cellAppend(CellPoint point, QList<int> listadd);
        int bucket_to_row(int bucket_row) const;
        int bucket_to_column(int bucket_column) const;
        /**
        * Renumber cells from start by adding adjustment
        */
        void renumber_cells(int start, int adjustment);
        bool cellRemoveOne(long int row, long int column, int index);

        const QAbstractItemModel* model;
        QList<DatacubeSelection*> selection_models;
        Datacube::Aggregators row_aggregators;
        Datacube::Aggregators col_aggregators;
        QVector<unsigned> row_counts; // list counting number of items in each row indexed by bucket number
        QVector<unsigned> col_counts;
        Datacube::Filters filters;
        typedef QHash<long, QList<int> > cells_t;
        cells_t cells; // maps from cell index (computed from bucket coordinates) to lists of indexes in underlying model
        typedef QHash<int, Cell> reverse_index_t;
        reverse_index_t reverse_index; // maps from underlying model index to coordinates in datacube (in buckets)

        void remove(int index);
        void add(int index);
        void split_row(int headerno, AbstractAggregator::Ptr aggregator);
        void split_column(int headerno, AbstractAggregator::Ptr aggregator);
        void aggregator_category_added(AbstractAggregator::Ptr aggregator, int headerno, int index, Qt::Orientation orientation);
        void aggregator_category_removed(AbstractAggregator::Ptr aggregator, int headerno, int index, Qt::Orientation orientation);

        /**
        * @returns the number of buckets (i.e. sections including empty sections) in datacube for
        * @param orientation
        */
        int number_of_buckets(Qt::Orientation orientation) const;

        /**
        * @return elements for bucket row, bucket column
        */
        QList<int> elements_in_bucket(int row, int column) const;

        /**
        * @return the bucket for row
        */
        int bucket_for_row(int row) const;

        /**
        * @return the bucket for column
        */
        int bucket_for_column(int column) const;

        /**
        * @return section for bucket row
        */
        int section_for_bucket_row(int bucket_row) const;

        /**
        * @return section for bucket row
        */
        int section_for_bucket_column(int bucket_column) const;

        /**
        * find cell_t with bucket for element
        * @param element element to look for
        * @param result the cell with the element, or an invalid cell
        * The strange interface is to avoid exporting cell_t
        */
        void bucket_for_element(int element, qdatacube::Cell& result) const;

        /**
        * Add a selection model for bucket change notification
        */
        void add_selection_model(DatacubeSelection* selection);

        /**
        * @returns true if included by the current set of filters
        */
        bool filtered_in(int element) const;
    public Q_SLOTS:
        void update_data(QModelIndex topleft, QModelIndex bottomRight);
        void remove_data(QModelIndex parent, int start, int end);
        void insert_data(QModelIndex parent, int start, int end);
        void slot_columns_changed(int column, int count);
        void slot_rows_changed(int row, int count);
        void slot_aggregator_category_added(int index);
        void slot_aggregator_category_removed(int);
        void remove_selection_model(QObject* selection_model);
};

}

#endif // QDATACUBE_DATACUBE_P_H
