#include "danishnamecube.h"
#include <QTextStream>
#include <QFile>
#include <QStandardItemModel>
#include <QDebug>

#include "columnaggregator.h"
#include "datacube.h"
#include "modeltest.h"

using namespace qdatacube;

QDebug operator<<( QDebug d, qdatacube::Datacube::HeaderDescription desc) {
    d << desc.categoryIndex << desc.span;
    return d;
}

int danishnamecube_t::printdatacube(const qdatacube::Datacube* datacube) {
  qDebug() << datacube->columnCount() << ", " << datacube->rowCount();
  qDebug() << datacube->headerCount(Qt::Horizontal) << ", " << datacube->headerCount(Qt::Vertical);
  qDebug() << "column headers";
  for (int i=0; i< datacube->headerCount(Qt::Horizontal); ++i) {
    qDebug() << datacube->headers(Qt::Horizontal, i);
  }
  qDebug() << "row headers";
  for (int i=0; i< datacube->headerCount(Qt::Vertical); ++i) {
    qDebug() << datacube->headers(Qt::Vertical, i);
  }
  int total = 0;
  for (int r = 0; r< datacube->rowCount(); ++r) {
    QString row_display;
    QTextStream row_display_stream(&row_display);
    for (int c = 0; c< datacube->columnCount(); ++c) {
      int count = datacube->elementCount(r,c);
      total += count;
      row_display_stream << datacube->elementCount(r,c) << "\t";
    }
    qDebug() << row_display;
  }

  return total;
}

void danishnamecube_t::load_model_data(QString fileName) {
    QFile data(fileName);
    data.open(QIODevice::ReadOnly);
  while (!data.atEnd()) {
    QString line = QString::fromLocal8Bit(data.readLine());
    QStringList columns = line.split(' ');
    Q_ASSERT(columns.size() == N_COLUMNS);
    QList<QStandardItem*> cell_items;
    Q_FOREACH(QString cell, columns) {
      cell.remove("\n");
      cell_items << new QStandardItem(cell);
    }
    m_underlying_model->appendRow(cell_items);
  }
  qDebug() << "Read " << m_underlying_model->rowCount() << " rows" << "with" << m_underlying_model->columnCount() << "columns";

}

danishnamecube_t::danishnamecube_t(QObject* parent):
    QObject(parent),
    m_underlying_model(new QStandardItemModel(0, N_COLUMNS, this))
{
    QStringList labels;
    labels << "firstname" << "lastname" << "sex" << "age" << "weight" << "kommune";
    m_underlying_model->setHorizontalHeaderLabels(labels);
    first_name_aggregator = AbstractAggregator::Ptr(new ColumnAggregator(m_underlying_model, FIRST_NAME));
    last_name_aggregator = AbstractAggregator::Ptr(new ColumnAggregator(m_underlying_model, LAST_NAME));
    sex_aggregator = QSharedPointer<ColumnAggregator>(new SexAggregator(m_underlying_model, SEX));
    age_aggregator = AbstractAggregator::Ptr(new ColumnAggregator(m_underlying_model, AGE));
    weight_aggregator = AbstractAggregator::Ptr(new ColumnAggregator(m_underlying_model, WEIGHT));
    kommune_aggregator = AbstractAggregator::Ptr(new ColumnAggregator(m_underlying_model, KOMMUNE));
    new ModelTest(m_underlying_model);
}

QStandardItemModel* danishnamecube_t::copy_model() {
  QStandardItemModel* rv = new QStandardItemModel(0, N_COLUMNS, m_underlying_model);
  for (int r=0; r<m_underlying_model->rowCount(); ++r) {
    QList<QStandardItem*> row;
    for (int c=0; c<m_underlying_model->columnCount(); ++c) {
      row << new QStandardItem(m_underlying_model->item(r,c)->text());
    }
    rv->appendRow(row);
  }
  QStringList header_labels;
  for (int c=0; c<m_underlying_model->columnCount(); ++c) {
    header_labels  << m_underlying_model->headerData(c, Qt::Horizontal).toString();
  }
  rv->setHorizontalHeaderLabels(header_labels);
  return rv;
};

#include "danishnamecube.moc"
