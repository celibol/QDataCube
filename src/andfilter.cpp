#include "andfilter.h"
#include <QSharedPointer>

namespace qdatacube {
class AndFilterPrivate {
public:
    AndFilterPrivate() {};
    QList<AbstractFilter::Ptr> m_filterComponents;
};

AndFilter::AndFilter(QAbstractItemModel* underlyingModel): AbstractFilter(underlyingModel), d(new AndFilterPrivate()) {}

void AndFilter::addFilter(AbstractFilter::Ptr filter) {
    Q_ASSERT(filter->underlyingModel() == underlyingModel());
    d->m_filterComponents.append(filter);
}

bool AndFilter::operator()(int row) const {
    Q_FOREACH(AbstractFilter::Ptr filter, d->m_filterComponents) {
       if(!(*filter)(row))  {
           return false;
       }
    }
    return true;
}

AndFilter::~AndFilter() {}

}

#include "andfilter.moc"
