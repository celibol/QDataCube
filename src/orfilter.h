#ifndef OR_FILTER_H
#define OR_FILTER_H

#include "abstractfilter.h"

class QAbstractItemModel;

namespace qdatacube {

class OrFilterPrivate;
/**
 * A filter allowing multiple filters to be combined, by default by OR'ing the
 * result of their operator().
 */
class QDATACUBE_EXPORT OrFilter : public AbstractFilter {
    Q_OBJECT
    public:
        explicit OrFilter(QAbstractItemModel* underlyingModel);

        /**
         * @return true if row is to be included
         */
        virtual bool operator()(int row) const;

        /**
         * adds a filter
         * Note: all filters need to share the same underlyingModel
         */
        void addFilter(AbstractFilter::Ptr filter);

        /**
         * dtor
         */
        virtual ~OrFilter();
    private:
        QScopedPointer<OrFilterPrivate> d;

};
}
#endif // OR_FILTER_H
