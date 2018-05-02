#ifndef Q_DATACUBE_DATACUBE_H
#define Q_DATACUBE_DATACUBE_H

#include "qdatacube_export.h"
#include "abstractaggregator.h"
#include "abstractfilter.h"

#include <QObject>
#include <QPair>

class QAbstractItemModel;
namespace qdatacube {
class Cell;
class DatacubePrivate;
}

namespace qdatacube {

/**
 * row: row in datacube
 * column: column in datacube
 * section: row or column in datacube
 * element: row number in underlying model
 * bucket(no): "raw" section, that is, including autocollapsed (empty) rows and columns
 */
class QDATACUBE_EXPORT Datacube : public QObject {
    Q_OBJECT
    public:
        /**
         * Construct simple 2-dimensional datacube with the 2 aggregators
         * @param underlying_model the model whose rows are the data elements in the datacube
         * @param row_aggregator initial aggregator.
         * @param column_aggregator initial aggregator.
         */
        Datacube(const QAbstractItemModel* model,
                AbstractAggregator::Ptr row_aggregator,
                AbstractAggregator::Ptr column_aggregator,
                QObject* parent = 0);

        /**
         * Construct datacube with no aggregators at all.
         */
        explicit Datacube(const QAbstractItemModel* model, QObject* parent = 0);

        /**
         * Destructor
         */
        ~Datacube();

        /**
         * Return the number of headers in the given direction. The sum of the
         * headers in both directions would give the total dimension of the (hyper)cube.
         * In other words, this is the number of dimensions projected to the orientation.
         */
        int headerCount(Qt::Orientation orientation) const;

        /**
         * Return the number of rows in the cube
         */
        int rowCount() const;

        /**
         * Return the number of columns in the cube
         */
        int columnCount() const;

        /**
         * small struct to represent a description of a header, ready to query the aggregator for details
         * and to know how wide a header is.
         */
        struct HeaderDescription {
            HeaderDescription(int cat, int span) : categoryIndex(cat), span(span) {
            }
            int categoryIndex;
            int span;
        };
        /**
         * @return pair of (category index for header, number of columns spanned)
         * @param orientation
         * @param index 0 is first header, 1 is next and so on, up until headerCount(orientation)
         * This function is meant to be convenient for drawing and similar.
         */
        QList<HeaderDescription> headers(Qt::Orientation orientation, int index) const;

        /**
         * @return the header section correcsponding to section in the datacube.
         * E.g., if a cube had 2 levels of headers, with the first header dividing
         * the cube in 2 equal parts, any section in the first part would return 0,
         * and the other part 1 for to_header_section(orientation, 0, section);
         * This is sort-of the reverse for to_section.
         */
        int toHeaderSection(Qt::Orientation orientation, int headerno, int section) const;

        /**
         * @return the range of section corresponding to a given header section
         * The range is given as a pair, (leftmost section, rightmost section)
         * The reverse is to_header_section
         */
        QPair<int,int> toSection(Qt::Orientation orientation, int headerno, int header_section) const;

        /**
         * @returns The number of elements for the given row, column.
         * equivalent to elements(row,column).size()
         */
        int elementCount(int row, int column) const;

        /**
         * @returns The elements in the given row, column
         */
        QList<int> elements(int row, int column) const;

        /**
         * @return number of elements corresponding to header section
         * equivalent (but much faster) to elements(direction, headerno, section).size()
         * @param orientation Qt::Vertical for rows, Qt::Horizontal for columns
         * @param headerno index of header, 0 for top or leftmost
         * @param section section of header. For headerno!=0 header section is not necessarily the same as item section
         */
        int elementCount(Qt::Orientation orientation, int headerno, int header_section) const;

        /**
         * @return elements corresponding to header section
         * @param orientation Qt::Vertical for rows, Qt::Horizontal for columns
         * @param headerno index of header, 0 for top or leftmost
         * @param section section of header. For headerno!=0 header section is not necessarily the same as item section
         */
        QList< int > elements(Qt::Orientation orientation, int headerno, int header_section) const;

        /**
         * @return the total number of (non-filtered) elements
         */
        int elementCount() const;

        /**
         * @return all non-filtered elements
         */
        QList<int> elements() const;

        /**
         * @returns the category index
         */
        int categoryIndex(Qt::Orientation orientation, int header_index, int section) const;

        /**
         * Add filter.
         */
        void addFilter(AbstractFilter::Ptr filter);

        /**
         * Remove all filters
         */
        void resetFilter();

        /**
         * Remove filter from list
         * \return true if filter was found and removed, and false if not found
         */
        bool removeFilter(AbstractFilter::Ptr filter);

        /**
         * Split header with aggregator.
         * @param orientation split by column or row
         * @param headerno which header to split. 0 means this is the new topmost split,
         *                 header_count(orientation) is the bottommost.
         * @param aggregator aggregator to use. Each non-empty category will give a new row or column
         */
        void split(Qt::Orientation orientation, int headerno, AbstractAggregator::Ptr aggregator);

        /**
         * Collapse header, removing it from datacube.
         * @param orientation collapse row or column
         * @param headerno which header to remove. Must be less that header_count(orientation)
         */
        void collapse(Qt::Orientation orientation, int headerno);

        /**
         * @returns the section (i.e, row for Qt::Vertical and column for Qt::Horizontal) for
         * @param orientation
         * and
         * @param element
         */
        int sectionForElement(int element, Qt::Orientation orientation) const;

        /**
         * @return the current section for the element (which might be wrong if you are listening to e.g. rowsAboutToBeRemoved)
         * Used for tests.
         * TODO: Find a better name for this.
         */
        int internalSection(int element, Qt::Orientation orientation) const;

        typedef QList< AbstractFilter::Ptr > Filters;
        typedef QList< AbstractAggregator::Ptr > Aggregators;

        /**
         * @return Return all filters in effect with their categories
         */
        Filters filters() const;

        /**
         * @return list of column aggregators, in order
         */
        Aggregators columnAggregators() const;

        /**
         * @return list of row aggregators, in order
         */
        Aggregators rowAggregators() const;

        /**
         * @return the underlying model
         */
        const QAbstractItemModel* underlyingModel() const;

        /**
         * Debug function: Dump internal state
         */
        void dump(bool cells, bool rowcounts, bool col_counts) const;

        /**
         * Run some internal checks. Useful for debugging
         */
        void check() const;

    Q_SIGNALS:
        /**
         * rows are about to be removed
         */
        void rowsAboutToBeRemoved(int index, int count);

        /**
         * columns are about to be removed
         */
        void columnsAboutToBeRemoved(int index, int count);

        /**
         * rows have been removed
         */
        void rowsRemoved(int index, int count);

        /**
         * columns have been removed
         */
        void columnsRemoved(int index, int count);

        /**
         * rows are about to be removed
         */
        void rowsAboutToBeInserted(int index, int count);

        /**
         * columns are about to be added
         */
        void columnsAboutToBeInserted(int index, int count);

        /**
         * rows have been added
         */
        void rowsInserted(int index, int count);

        /**
         * columns have been added
         */
        void columnsInserted(int index, int count);

        /**
         * header data has changed
         * data_changed will still be emitted
         */
        void headersChanged(Qt::Orientation, int first, int last);

        /**
         * The value in cell has changed
         */
        void dataChanged(int row,int column);

        /**
         * Datacube is about to be completely changed. All cell and headers can change
         */
        void aboutToBeReset();

        /**
         * Datacube has completely changed. All cell and headers can change
         */
        void reset();

        /**
         * filtering changed
         */
        void filterChanged();

    private:
        QScopedPointer<DatacubePrivate> d;
        friend class DatacubeSelection;
        friend class DatacubeSelectionPrivate;

};
}

#endif // Q_DATACUBE_DATACUBE_H
