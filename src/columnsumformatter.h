#ifndef QDATACUBE_COLUMN_FORMATTER_H
#define QDATACUBE_COLUMN_FORMATTER_H

#include "abstractformatter.h"
#include "qdatacube_export.h"
namespace qdatacube {

/**
  * Simple demonstration formatter that takes a column and uses the sum of that for display
  */
class ColumnSumFormatterPrivate;
class QDATACUBE_EXPORT ColumnSumFormatter : public AbstractFormatter {
    public:
        /**
         * @param underlying_model the underlying_model
         * @param column the column of the underlying model we are summing over. Should provide data convertible to double
         * @param precision precision of the output
         * @param scale this is multiplied on the sum before displaying (e.g. using .001 to convert kg to T)
         * @param suffix a (small) string that is appended to the format, e.g. "t" for tonnes.
         */
        ColumnSumFormatter(QAbstractItemModel* underlying_model, qdatacube::DatacubeView* view, int column, int precision, QString suffix, double scale = 1.0 );
        virtual QString format(QList< int > rows) const;
        virtual ~ColumnSumFormatter();
    protected:
        virtual void update(UpdateType element);
    private:
        QScopedPointer<ColumnSumFormatterPrivate> d;
};
} // end of namespace
#endif // COLUMN_FORMATTER_H
