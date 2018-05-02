#ifndef QDATACUBE_COUNT_FORMATTER_H
#define QDATACUBE_COUNT_FORMATTER_H

#include "abstractformatter.h"
#include "qdatacube_export.h"

namespace qdatacube {

/**
 * Simplest formatter there is: Return the count of each cell/header
 */
class QDATACUBE_EXPORT CountFormatter : public AbstractFormatter {
    public:
        /**
         * @param underlyingModel: the source model
         * @param view: the qdatacube view (optional, used to indicate cell size)
         * @param multiplier: multiply all the figures in the output by this constant
         */
        CountFormatter(QAbstractItemModel* underlyingModel, qdatacube::DatacubeView* view = 0L, const double multiplier = 1.0);
        virtual QString format(QList< int > rows) const;
    protected:
        virtual void update(qdatacube::AbstractFormatter::UpdateType updateType);
    private:
        const double m_multiplier;
};
}
#endif // COUNT_FORMATTER_H
