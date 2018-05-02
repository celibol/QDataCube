#include "abstractformatter.h"
#include "datacubeview.h"
#include <stdexcept>
#include <QEvent>

namespace qdatacube {


class AbstractFormatterPrivate {
    public:
        AbstractFormatterPrivate(QAbstractItemModel* underlying_model, DatacubeView* view) : m_underlying_model(underlying_model), m_view(view), m_cell_size(10,10) {
            m_name = "unnamed";
            m_shortName = "N/A";
        }
        QAbstractItemModel* m_underlying_model;
        DatacubeView* m_view;
        QSize m_cell_size;
        QString m_name;
        QString m_shortName;
};
AbstractFormatter::AbstractFormatter(QAbstractItemModel* underlying_model, DatacubeView* view)
 : QObject(view), d(new AbstractFormatterPrivate(underlying_model,view))
{
    if (!underlying_model) {
        throw std::runtime_error("underlying_model must be non-null");
    }
    if(view) {
        view->installEventFilter(this);
    }
}

QSize AbstractFormatter::cellSize() const
{
  return d->m_cell_size;
}

void AbstractFormatter::setCellSize(QSize size)
{
  if (d->m_cell_size != size) {
    d->m_cell_size = size;
    emit cellSizeChanged(size);
  }
}

DatacubeView* AbstractFormatter::datacubeView() const {
    return d->m_view;
}

QAbstractItemModel* AbstractFormatter::underlyingModel() const {
    return d->m_underlying_model;
}

bool AbstractFormatter::eventFilter(QObject* filter , QEvent* event ) {
    if(filter == datacubeView() && event->type() == QEvent::FontChange) {
        update(qdatacube::AbstractFormatter::CellSize);
    }
    return QObject::eventFilter(filter, event);
}

void AbstractFormatter::update(AbstractFormatter::UpdateType element) {
    Q_UNUSED(element);
    // do nothing
}

QString AbstractFormatter::name() const {
    return d->m_name;
}

void AbstractFormatter::setName(const QString& newName) {
    d->m_name = newName;
}

void AbstractFormatter::setShortName(const QString& newShortName) {
    d->m_shortName = newShortName;
}

QString AbstractFormatter::shortName() const {
    return d->m_shortName;
}

AbstractFormatter::~AbstractFormatter()
{

}


}

#include "abstractformatter.moc"
