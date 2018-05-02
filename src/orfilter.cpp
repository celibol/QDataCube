#include "orfilter.h"
#include <QSharedPointer>

namespace qdatacube {
class OrFilterPrivate {
public:
    OrFilterPrivate() {};
    QList<AbstractFilter::Ptr> m_filterComponents;
};

OrFilter::OrFilter(QAbstractItemModel* underlyingModel): AbstractFilter(underlyingModel), d(new OrFilterPrivate()) {}

void OrFilter::addFilter(AbstractFilter::Ptr filter) {
    Q_ASSERT(filter->underlyingModel() == underlyingModel());
    d->m_filterComponents.append(filter);
}

bool OrFilter::operator()(int row) const {
    Q_FOREACH(AbstractFilter::Ptr filter, d->m_filterComponents) {
       if((*filter)(row))  {
           return true;
       }
    }
    return false;
}

OrFilter::~OrFilter() {}

}

#include "orfilter.moc"
