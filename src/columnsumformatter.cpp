#include "columnsumformatter.h"
#include <QAbstractItemModel>
#include <QEvent>
#include <stdexcept>
#include <QWidget>
#include "datacubeview.h"
namespace qdatacube {

class ColumnSumFormatterPrivate {
    public:
        ColumnSumFormatterPrivate(int column, int precision, QString suffix, double scale) : m_column(column), m_precision(precision), m_suffix(suffix), m_scale(scale) {
        }
        const int m_column;
        const int m_precision;
        QString m_suffix;
        const double m_scale;
};

ColumnSumFormatter::ColumnSumFormatter(QAbstractItemModel* underlying_model, qdatacube::DatacubeView* view, int column, int precision, QString suffix, double scale)
 : AbstractFormatter(underlying_model, view), d(new ColumnSumFormatterPrivate(column, precision, suffix, scale))
{
  if (column >= underlying_model->columnCount()|| column<0) {
    throw std::runtime_error(QString("Column %1 must be in the underlying model, ie., be between 0 and %2").arg(column).arg(underlying_model->columnCount()).toStdString());
  }
  update(qdatacube::AbstractFormatter::CellSize);
  setShortName("SUM");
  setName(QString("Sum over %1").arg(underlyingModel()->headerData(d->m_column, Qt::Horizontal).toString()));
}

QString ColumnSumFormatter::format(QList< int > rows) const
{
  double accumulator = 0;
  Q_FOREACH(int element, rows) {
    accumulator += underlyingModel()->index(element, d->m_column).data().toDouble();
  }
  return QString::number(accumulator*d->m_scale,'f',d->m_precision) + d->m_suffix;
}
void ColumnSumFormatter::update(AbstractFormatter::UpdateType element) {
    if(element == qdatacube::AbstractFormatter::CellSize) {
        if(datacubeView()) {
            // Set the cell size, by summing up all the data in the model, and using that as input
            double accumulator = 0;
            for (int element = 0, nelements = underlyingModel()->rowCount(); element < nelements; ++element) {
                accumulator += underlyingModel()->index(element, d->m_column).data().toDouble();
            }
            QString big_cell_contents = QString::number(accumulator*d->m_scale, 'f', d->m_precision) + d->m_suffix;
            setCellSize(QSize(datacubeView()->fontMetrics().width(big_cell_contents), datacubeView()->fontMetrics().lineSpacing()));
        }
    }
}

ColumnSumFormatter::~ColumnSumFormatter() {

}



} // end of namespace
