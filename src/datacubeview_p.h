#ifndef QDATACUBE_DATACUBEVIEW_P
#define QDATACUBE_DATACUBEVIEW_P
#include <QObject>
#include <QSize>
#include <QPoint>
#include <QRect>
#include "cell.h"

class QPaintEvent;
namespace qdatacube {

class DatacubeSelection;
class AbstractFormatter;
class Datacube;
class DatacubeView;

class DatacubeViewPrivate : public QObject {
    Q_OBJECT
    public:
        DatacubeViewPrivate(DatacubeView* datacubeview);
        DatacubeView* q;
        Datacube* datacube;
        DatacubeSelection* selection;
        QList<AbstractFormatter*> formatters;
        int horizontal_header_height;
        int vertical_header_width;
        QSize cell_size;
        QSize datacube_size;
        QSize visible_cells;
        QPoint mouse_press_point;
        QPoint mouse_press_scrollbar_state;
        QPoint last_mouse_press_point;
        QPoint last_mouse_press_scrollbar_state;
        QRect selection_area;
        QRect header_selection_area;
        bool show_totals;

        /**
         *  Return cell corresponding to position. Note that position is zero-based, and if no cells at position an
         * invalid cell_t is returned (i.e., cell_for_position(outside_pos).invalid() == true );
         **/
        Cell cell_for_position(QPoint pos, int vertical_scrollbar_value, int horizontal_scrollbar_value) const;
        void paint_datacube(QPaintEvent* event) const;
    public Q_SLOTS:
        void relayout();
        void datacube_deleted();
};

}

#endif // QDATACUBE_DATACUBEVIEW_P
