#include "countformatter.h"
#include <QAbstractItemModel>
#include <QEvent>
#include "datacubeview.h"

namespace qdatacube {

CountFormatter::CountFormatter(QAbstractItemModel* underlying_model, qdatacube::DatacubeView* view, const double multiplier)
  : AbstractFormatter(underlying_model, view),
    m_multiplier(multiplier)
{
    update(qdatacube::AbstractFormatter::CellSize);
    setName(tr("Count"));
    setShortName("#");
}

QString CountFormatter::format(QList< int > rows) const {
  return QString::number(rows.size()*m_multiplier);
}

void CountFormatter::update(AbstractFormatter::UpdateType updateType) {
    if(updateType == qdatacube::AbstractFormatter::CellSize) {
        if(datacubeView()) {
            QString big_cell_contents = QString::number(underlyingModel()->rowCount());
            setCellSize(QSize(datacubeView()->fontMetrics().width(big_cell_contents),
                            datacubeView()->fontMetrics().ascent()+1));
        }
    }
}

}

