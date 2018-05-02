#ifndef QDATACUBE_DATACUBESELECTION_P
#define QDATACUBE_DATACUBESELECTION_P

#include <QObject>
#include <QHash>
#include <QItemSelectionModel>

class QItemSelectionModel;
namespace qdatacube {

class DatacubeSelection;

class Datacube;

class DatacubeSelectionPrivate : public QObject {
    Q_OBJECT
    public:
        DatacubeSelectionPrivate(DatacubeSelection* datacubeselection);
        DatacubeSelection* q;
        Datacube* datacube;
        QHash<long, int> cells; // maps from bucket index to number of selected items
        QSet<int> selected_elements; // set of the selected rows in the underlying model from the datacube
        QItemSelectionModel* synchronized_selection_model;
        bool ignore_synchronized;
        int nrows; // bucket size of datacube
        int ncolumns; // bucket size of datacube

        /**
         * \return the value in cell \param row, \param value
         */
        int cellValue(int row, int column) const {
            return cells.value(row+column*nrows,0);
        }
        /**
         * \param row row to decrease
         * \param column column to decrease
         * \param value how much to decrease with
         * \return the new value
         */
        int decreaseCell(int row, int column, int value = 1) {
            QHash<long,int>::iterator it = cells.find(row+column*nrows);
            if(it == cells.end()) {
                Q_ASSERT(false);
                return 0;
            } else {
                *it-=value;
                if(*it == 0) {
                    cells.erase(it);
                    return 0;
                }
                return *it;
            }
        }
        /**
         * \param row row to increase
         * \param column column to increase
         * \param value how much to increase with
         * \return the new value in \param row
         */
        int increaseCell(int row, int column, int value = 1) {
            QHash<long,int>::iterator it = cells.find(row+column*nrows);
            if(it == cells.end()) {
                cells.insert(row+column*nrows,value);
                return value;
            } else {
                *it+=value;
                return *it;
            }
        }

        void dump();

        void select_on_synchronized(QList<int> elements);
        void deselect_on_synchronized(QList<int> elements);
        void clear_synchronized();
        QItemSelection map_to_synchronized(QList<int> elements);
        QList<int> elements_from_selection(QItemSelection selection);
        void datacube_adds_element_to_bucket(int row, int column, int element);
        void datacube_removes_element_from_bucket(int row, int column, int element);
        void datacube_deletes_elements(int start, int end);
        void datacube_inserts_elements(int start, int end);
    public Q_SLOTS:
        void reset();


};

} // namespace

#endif // QDATACUBE_DATACUBESELECTION_P
