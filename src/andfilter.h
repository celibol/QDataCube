#ifndef AND_FILTER_H
#define AND_FILTER_H

#include "abstractfilter.h"

class QAbstractItemModel;

namespace qdatacube {

class AndFilterPrivate;
/**
 * A filter allowing multiple filters to be combined, by AND'ing the
 * result of their operator().
 */
class QDATACUBE_EXPORT AndFilter : public AbstractFilter {
    Q_OBJECT
    public:
        explicit AndFilter(QAbstractItemModel* underlyingModel);

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
        virtual ~AndFilter();
    private:
        QScopedPointer<AndFilterPrivate> d;

};
}
#endif // AND_FILTER_H
