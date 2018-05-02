/*
 Author: Ange Optimization <contact@ange.dk>  (C) Ange Optimization ApS 2010

 Copyright: See COPYING file that comes with this distribution

*/

#ifndef DATACUBE_VIEW_H
#define DATACUBE_VIEW_H

#include <QAbstractScrollArea>
#include "qdatacube_export.h"

namespace qdatacube {

class AbstractFormatter;
class DatacubeSelection;
class Datacube;
class DatacubeViewPrivate;

class QDATACUBE_EXPORT DatacubeView : public QAbstractScrollArea  {
    Q_OBJECT
    public:
        DatacubeView(QWidget* parent = 0);
        virtual ~DatacubeView();

        /**
         * Set the datacube to be view
         * If the underlying model has changed, this will clear all the formatters
         */
        void setDatacube(Datacube* datacube);

        /**
         * @return current datacube
         */
        Datacube* datacube() const;

        /**
         * @return current datacube_selection
         */
        DatacubeSelection* datacubeSelection() const;

        /**
         * Add formatter below each cell
         * Ownership over summarizer will be claimed
         */
        void addFormatter(AbstractFormatter* formatter);

        /**
         * Take formatter by index, clearning parent
         * @return the formatter taken.
         */
        AbstractFormatter* takeFormatter(int index);

        /**
         * @return all the formatters in current use
         */
        QList<AbstractFormatter*> formatters() const;
    protected:
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event );
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void resizeEvent(QResizeEvent* event);
        virtual bool event(QEvent* event);

        /**
         * @return rectangle describing the corner area
         */
        QRect corner() const;
    Q_SIGNALS:
        void verticalHeaderContextMenu(QPoint pos,int level,int section);
        void horizontalHeaderContextMenu(QPoint pos, int level, int section);
        void cornerContextMenu(QPoint pos);
        void cellContextMenu(QPoint pos, int row, int column);
    protected:
        virtual bool viewportEvent(QEvent* event);
        virtual void contextMenuEvent(QContextMenuEvent* event);
    private:
        QScopedPointer<DatacubeViewPrivate> d;
        friend class DatacubeViewPrivate;

};

}
#endif // DATACUBE_VIEW_H
