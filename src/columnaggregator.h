/*
 Author: Ange Optimization <contact@ange.dk>  (C) Ange Optimization ApS 2010

 Copyright: See COPYING file that comes with this distribution

*/

#ifndef COLUMN_FILTER_H
#define COLUMN_FILTER_H

#include "abstractaggregator.h"

#include <QScopedPointer>

#include "qdatacube_export.h"
#include <QAbstractItemModel>

class QModelIndex;
namespace qdatacube {

/**
 * \brief Aggregates based on contents in a given column in the underlying model.
 *
 * Aggregation happens based on unique string content in the given column.
 *
 * For example, a column with a content like
 *
 * London
 * Newcastle
 * London
 * York
 *
 * will report 3 categories, and place row 0 and 2 in the London category, row 1
 * in the Newcastle category and row 3 in the York category.
 */

class ColumnAggregatorPrivate;
class QDATACUBE_EXPORT ColumnAggregator : public AbstractAggregator {
    Q_OBJECT
    public:
        ColumnAggregator(const QAbstractItemModel* model,  int section);
        ~ColumnAggregator();
        virtual int operator()(int row) const;
        /**
         * Return section
         */
        int section() const;

        virtual int categoryCount() const;

        virtual QVariant categoryHeaderData(int category, int role = Qt::DisplayRole) const;

        /**
         * Trim categories from the right to max max_chars characters.
         * NOTICE This also trims existing categories in spite of the functions name.
         * WARNING Do not call this on an aggregator that is in use as the datacube is not build for categories to
         *         change for data items except when the item is changing itself.
         **/
        void setTrimNewCategoriesFromRight(int max_chars);
    public Q_SLOTS:
        /**
         * Recalculate categories. This is also triggered automatically when the number of changed or removed rows
         * exceeds the half the current size
         */
        void resetCategories();
    private:
        QScopedPointer<ColumnAggregatorPrivate> d;
        friend class ColumnAggregatorPrivate;
    private Q_SLOTS:
        void refresh_categories_in_rect(QModelIndex top_left, QModelIndex bottom_right);
        void add_rows_to_categories(const QModelIndex& parent, int start, int end);
        void remove_rows_from_categories(const QModelIndex& parent, int start, int end);
};

}
#endif // COLUMN_FILTER_H
