#ifndef DATACUBE_SELECTION_H
#define DATACUBE_SELECTION_H

#include "qdatacube_export.h"

#include <QItemSelection>
#include <QObject>

class QItemSelectionModel;
namespace qdatacube {
class Datacube;
class DatacubeSelectionPrivate;
class DatacubeView;
}

namespace qdatacube {

/**
 * Handles tracking of selection on a datacube
 * The selection is automatically sync'ed to the
 * datacube's underlying model.
 *
 *
 * Usually, there is no need to instantiate this class manually, as one is created automatically by
 * the datacube_view_t
 */
class QDATACUBE_EXPORT DatacubeSelection : public QObject {
    Q_OBJECT
    public:
        DatacubeSelection(qdatacube::Datacube* datacube, DatacubeView* view);
        virtual ~DatacubeSelection();

        enum SelectionStatus {
            UNSELECTED = 0,
            PARTIALLY_SELECTED = 1,
            SELECTED = 2
        };

        /**
         * Clears selection
         */
        void clear();

        /**
         * Add elements to selection
         */
        void addElements(QList<int> elements);

        /**
         * Remove elements from selection
         **/
        void removeElements(QList<int> elements);

        /**
         * Add cell to selection
         */
        void addCell(int row, int column);

        /**
         * Get selection status from (view) cell
         */
        SelectionStatus selectionStatus(int row, int column) const;

        /**
         * Synchronize with specified model. Any previous sync. is deleted, and
         * passing null will remove any synchronization
         **/
        void synchronizeWith(QItemSelectionModel* synchronized_selection_model);

    Q_SIGNALS:
        /**
         * Selection status has changed for cell
         **/
        void selectionStatusChanged(int row, int column);

    public Q_SLOTS:
        /**
         * Update the selection by adding the items in select to the selection and removing the items in deselect
         **/
        void updateSelection(QItemSelection select,QItemSelection deselect);

    private:
        friend class Datacube;
        friend class DatacubePrivate;
        QScopedPointer<DatacubeSelectionPrivate> d;
};

}
#endif // DATACUBE_SELECTION_H
