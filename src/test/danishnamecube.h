#ifndef DANISHNAMECUBE_H
#define DANISHNAMECUBE_H

#include <QObject>
#include <QSharedPointer>

class QStandardItemModel;
class QAbstractItemModel;
namespace qdatacube {
class Datacube;
class AbstractAggregator;
}

#include <columnaggregator.h>
#include <QColor>
class SexAggregator : public qdatacube::ColumnAggregator {
    public:
        SexAggregator(QAbstractItemModel* model, int section) : ColumnAggregator(model,section) {

        }
        virtual QVariant categoryHeaderData(int category, int role = Qt::DisplayRole) const {
            QVariant displayrole = ColumnAggregator::categoryHeaderData(category,Qt::DisplayRole);
            if(role == Qt::DisplayRole) {
                return displayrole;
            }
            if(role == Qt::BackgroundRole) {
                QString displayrolestring = displayrole.toString();
                if(displayrolestring == "male") {
                    return QColor(Qt::cyan);
                } else if(displayrolestring == "female") {
                    return QColor(Qt::magenta);
                }
                return QColor(Qt::yellow);
            }
            if(role == Qt::ForegroundRole) {
                QString displayrolestring = displayrole.toString();
                if(displayrolestring == "male") {
                    return QColor(Qt::blue);
                } else if(displayrolestring == "female") {
                    return QColor(Qt::red);
                }
                return QColor(Qt::gray);

            }
            if(role == Qt::ToolTipRole) {
                return displayrole;
            }
            return QVariant();
        }
};


class danishnamecube_t : public QObject {
  Q_OBJECT
public:
    danishnamecube_t(QObject* parent = 0);
    static int printdatacube(const qdatacube::Datacube* datacube);
    void load_model_data(QString filename);
    /**
     * Return a deep copy of the underlying model (to test manipulation functions)
     */
    QStandardItemModel* copy_model();
    QStandardItemModel* m_underlying_model;
    qdatacube::AbstractAggregator::Ptr first_name_aggregator;
    qdatacube::AbstractAggregator::Ptr last_name_aggregator;
    QSharedPointer<qdatacube::ColumnAggregator> sex_aggregator;
    qdatacube::AbstractAggregator::Ptr age_aggregator;
    qdatacube::AbstractAggregator::Ptr weight_aggregator;
    qdatacube::AbstractAggregator::Ptr kommune_aggregator;
    enum columns_t {
      FIRST_NAME,
      LAST_NAME,
      SEX,
      AGE,
      WEIGHT,
      KOMMUNE,
      N_COLUMNS
    };
};

#endif // DANISHNAMECUBE_H
