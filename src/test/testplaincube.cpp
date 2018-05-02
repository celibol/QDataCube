/*
 Author: Ange Optimization <contact@ange.dk>  (C) Ange Optimization ApS 2010

 Copyright: See COPYING file that comes with this distribution

*/

#include "testplaincube.h"
#include <QStandardItemModel>
#include <algorithm>
#include "datacube.h"
#include "columnaggregator.h"
#include "filterbyaggregate.h"

using namespace qdatacube;

void testplaincube::test_empty_cube() {
  Datacube datacube(m_underlying_model);
  QCOMPARE(datacube.headerCount(Qt::Horizontal),0);
  QCOMPARE(datacube.headerCount(Qt::Vertical),0);
  QCOMPARE(datacube.rowCount(),1);
  QCOMPARE(datacube.columnCount(),1);
  QCOMPARE(datacube.elementCount(0,0), m_underlying_model->rowCount());

  // Split and collapse it in both directions, then both at once
  datacube.split(Qt::Horizontal, 0, sex_aggregator);
  datacube.collapse(Qt::Horizontal,0);
  datacube.split(Qt::Vertical, 0, sex_aggregator);
  datacube.collapse(Qt::Vertical,0);
  datacube.split(Qt::Horizontal, 0, kommune_aggregator);
  datacube.split(Qt::Vertical, 0, sex_aggregator);
  datacube.collapse(Qt::Horizontal,0);
  datacube.collapse(Qt::Vertical,0);

  // Check for survival
  QCOMPARE(datacube.headerCount(Qt::Horizontal),0);
  QCOMPARE(datacube.headerCount(Qt::Vertical),0);
  QCOMPARE(datacube.rowCount(),1);
  QCOMPARE(datacube.columnCount(),1);
  QCOMPARE(datacube.elementCount(0,0), m_underlying_model->rowCount());


}

void testplaincube::testColumnAggregatornames() {
    QCOMPARE(sex_aggregator->name(),QLatin1String("sex"));
    QCOMPARE(kommune_aggregator->name(),QLatin1String("kommune"));

}


int findCategoryIndexForString(AbstractAggregator::Ptr aggregator, QString string) {
    int cat = -1;
    for(int i = 0 ; i < aggregator->categoryCount(); i++) {
        if(aggregator->categoryHeaderData(i).toString() == string) {
            cat = i;
        }
    }
    return cat;
}

void testplaincube::test_basics() {
  Datacube datacube(m_underlying_model, first_name_aggregator, last_name_aggregator);
  int larsen_cat = findCategoryIndexForString(last_name_aggregator, "Larsen");
  Q_ASSERT(larsen_cat != -1);
  int kim_cat = findCategoryIndexForString(first_name_aggregator, "Kim");
  Q_ASSERT(kim_cat != -1);
  QList<int> rows = datacube.elements(kim_cat, larsen_cat);
  Q_FOREACH(int row, rows) {
    QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, testplaincube::FIRST_NAME)).toString(), QString::fromLocal8Bit("Kim"));
    QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, testplaincube::LAST_NAME)).toString(), QString::fromLocal8Bit("Larsen"));
  }
  int total = 0;
  for (int row = 0; row < datacube.rowCount(); ++row) {
    for (int column = 0; column < datacube.columnCount(); ++column) {
      total += datacube.elementCount(row, column);
    }
  }
  QCOMPARE(total, m_underlying_model->rowCount());
  QCOMPARE(datacube.headerCount(Qt::Vertical), 1);
  QCOMPARE(datacube.headers(Qt::Vertical,0).size(), first_name_aggregator->categoryCount());
  QCOMPARE(datacube.headerCount(Qt::Horizontal), 1);
  QCOMPARE(datacube.headers(Qt::Horizontal,0).size(), last_name_aggregator->categoryCount());

}

void testplaincube::test_split() {
  Datacube datacube(m_underlying_model, last_name_aggregator, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1, sex_aggregator);
  QCOMPARE(datacube.headerCount(Qt::Horizontal), 2);
  QCOMPARE(datacube.headerCount(Qt::Vertical), 1);
  // Test headers
  int i = 0;
  Q_FOREACH(Datacube::HeaderDescription header, datacube.headers(Qt::Horizontal, 0)) {
    QCOMPARE(header.span,2);
    QCOMPARE(header.categoryIndex, i++);
  }

  i = 0;
  Q_FOREACH(Datacube::HeaderDescription header, datacube.headers(Qt::Horizontal, 1)) {
    QCOMPARE(header.span,1);
    QCOMPARE(header.categoryIndex, i++ % 2);
  }

  //Test sum of data
  int total = 0;
  for (int row = 0; row < datacube.rowCount(); ++row) {
    for (int column = 0; column < datacube.columnCount(); ++column) {
      total += datacube.elementCount(row, column);
    }
  }

  // Pick out a data point: All   male residents in Odense named Pedersen (2)
  int pedersencat = findCategoryIndexForString(last_name_aggregator, "Pedersen");
  int malecat = findCategoryIndexForString(sex_aggregator, "male");
  int odensecat = findCategoryIndexForString(kommune_aggregator, "Odense");
  int col = malecat+ sex_aggregator->categoryCount()*odensecat;
  QCOMPARE(datacube.elementCount(pedersencat, col), 2);

}

void testplaincube::dotest_splittwice(Qt::Orientation direction)
{
  // direction and parallel: direction to test
  // normal: the other direction
  AbstractAggregator::Ptr row_aggregator;
  AbstractAggregator::Ptr column_aggregator;
  Qt::Orientation normal;
  if (direction == Qt::Horizontal) {
    column_aggregator = kommune_aggregator;
    row_aggregator = last_name_aggregator;
    normal = Qt::Vertical;
  } else {
    row_aggregator = kommune_aggregator;
    column_aggregator = last_name_aggregator;
    normal = Qt::Horizontal;
  }
  Datacube datacube(m_underlying_model, row_aggregator, column_aggregator);
  datacube.split(direction, 1, sex_aggregator);
  datacube.split(direction, 1, age_aggregator);
  QCOMPARE(datacube.headerCount(direction),3);
  QCOMPARE(datacube.headerCount(normal),1);
  int total = 0;
  QList<Datacube::HeaderDescription > normal_headers = datacube.headers(normal, 0);
  QCOMPARE(normal_headers.size(), last_name_aggregator->categoryCount());
  QList<Datacube::HeaderDescription > parallel_headers0 = datacube.headers(direction, 0);
  const int nkommuner = kommune_aggregator->categoryCount();
  QCOMPARE(parallel_headers0.size(), nkommuner);
  QList<Datacube::HeaderDescription > parellel_headers1 = datacube.headers(direction, 1);
  QList<Datacube::HeaderDescription > parellel_headers2 = datacube.headers(direction, 2);
  QStringList parallel0_headers;
  AbstractAggregator::Ptr aggregator0 = (direction == Qt::Horizontal) ? datacube.columnAggregators().at(0) : datacube.rowAggregators().at(0);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(direction,0)) {
    for (int i=0; i<hp.span; ++i) {
      parallel0_headers << aggregator0->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList parellel1_headers;
  AbstractAggregator::Ptr aggregator1 = (direction == Qt::Horizontal) ? datacube.columnAggregators().at(1) : datacube.rowAggregators().at(1);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(direction,1)) {
    for (int i=0; i<hp.span; ++i) {
      parellel1_headers << aggregator1->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList parellel2_headers;
  AbstractAggregator::Ptr aggregator2 = (direction == Qt::Horizontal) ? datacube.columnAggregators().at(2) : datacube.rowAggregators().at(2);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(direction,2)) {
    for (int i=0; i<hp.span; ++i) {
      parellel2_headers << aggregator2->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  const int parellel_count = (direction == Qt::Horizontal) ? datacube.columnCount() : datacube.rowCount();
  const int normal_count = (direction == Qt::Vertical) ? datacube.columnCount() : datacube.rowCount();
  QCOMPARE(parellel2_headers.size(),parellel_count);
  for (int n = 0; n < normal_count; ++n) {
    for (int p = 0; p < parellel_count; ++p) {
      QList<int> elements = (direction == Qt::Horizontal) ? datacube.elements(n,p) : datacube.elements(p,n);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, LAST_NAME)).toString(), last_name_aggregator->categoryHeaderData(normal_headers.at(n).categoryIndex).toString());
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, KOMMUNE)).toString(), parallel0_headers.at(p));
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, AGE)).toString(), parellel1_headers.at(p));
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, SEX)).toString(), parellel2_headers.at(p));
      }
    }
  }
  QCOMPARE(total, 100);

}

void testplaincube::test_split_twice_horizontal() {
  dotest_splittwice(Qt::Horizontal);
}

void testplaincube::test_split_twice_vertical()
{
  dotest_splittwice(Qt::Vertical);
}

void testplaincube::test_collapse1() {
  Datacube datacube(m_underlying_model, last_name_aggregator, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1, sex_aggregator);
  datacube.collapse(Qt::Horizontal,0);
  QCOMPARE(datacube.headerCount(Qt::Horizontal),1);
  QCOMPARE(datacube.headerCount(Qt::Vertical),1);
  int total = 0;
  QList<Datacube::HeaderDescription > col_headers = datacube.headers(Qt::Horizontal, 0);
  AbstractAggregator::Ptr col_aggregator = datacube.columnAggregators().at(0);
  QList<Datacube::HeaderDescription > row_headers = datacube.headers(Qt::Vertical, 0);
  AbstractAggregator::Ptr row_aggregator = datacube.rowAggregators().at(0);
  for (int row = 0; row < datacube.rowCount(); ++row) {
    for (int column = 0; column < datacube.columnCount(); ++column) {
      Q_FOREACH(int cell, datacube.elements(row, column)) {
        ++total;
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, SEX)).toString(), col_aggregator->categoryHeaderData(col_headers.at(column).categoryIndex).toString());
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, LAST_NAME)).toString(), row_aggregator->categoryHeaderData(row_headers.at(row).categoryIndex).toString());
      }
    }
  }
  QCOMPARE(total, 100);

}

void testplaincube::test_collapse2() {
  Datacube datacube(m_underlying_model, last_name_aggregator, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1, sex_aggregator);
  datacube.collapse(Qt::Horizontal,1);
  QCOMPARE(datacube.headerCount(Qt::Horizontal),1);
  QCOMPARE(datacube.headerCount(Qt::Vertical),1);
  int total = 0;
  QList<Datacube::HeaderDescription > col_headers = datacube.headers(Qt::Horizontal, 0);
  AbstractAggregator::Ptr col_aggregator = datacube.columnAggregators().at(0);
  QList<Datacube::HeaderDescription > row_headers = datacube.headers(Qt::Vertical, 0);
  AbstractAggregator::Ptr row_aggregator = datacube.rowAggregators().at(0);
  for (int row = 0; row < datacube.rowCount(); ++row) {
    for (int column = 0; column < datacube.columnCount(); ++column) {
      Q_FOREACH(int cell, datacube.elements(row, column)) {
        ++total;
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, KOMMUNE)).toString(), col_aggregator->categoryHeaderData(col_headers.at(column).categoryIndex).toString());
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, LAST_NAME)).toString(), row_aggregator->categoryHeaderData(row_headers.at(row).categoryIndex).toString());
      }
    }
  }
  QCOMPARE(total, 100);

}

void testplaincube::test_collapse3() {
  do_testcollapse3(Qt::Horizontal);
}

void testplaincube::test_collapse3_vertical() {
  do_testcollapse3(Qt::Vertical);

}

void testplaincube::do_testcollapse3(Qt::Orientation orientation) {
  const bool horizontal = (orientation == Qt::Horizontal);
  const Qt::Orientation normal = horizontal ? Qt::Vertical : Qt::Horizontal;
  Datacube datacube(m_underlying_model, horizontal ? last_name_aggregator : kommune_aggregator, horizontal ? kommune_aggregator: last_name_aggregator);
  datacube.split(orientation, 1, age_aggregator);
  datacube.split(orientation, 2, sex_aggregator);
  datacube.collapse(orientation,1);
  QCOMPARE(datacube.headerCount(orientation),2);
  QCOMPARE(datacube.headerCount(normal),1);
  int total = 0;
  QList<Datacube::HeaderDescription > parallel_headers = datacube.headers(orientation, 1);
  AbstractAggregator::Ptr parallel_aggregator = (horizontal ? datacube.columnAggregators() : datacube.rowAggregators()).at(1);
  QList<Datacube::HeaderDescription > normal_headers = datacube.headers(normal, 0);
  AbstractAggregator::Ptr normal_aggregator = (!horizontal ? datacube.columnAggregators() : datacube.rowAggregators()).at(0);
  QStringList parallel_0_headers;
  AbstractAggregator::Ptr parallel_0_aggregator = (horizontal ? datacube.columnAggregators() : datacube.rowAggregators()).at(0);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(orientation,0)) {
    for (int i=0; i<hp.span; ++i) {
      parallel_0_headers << parallel_0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QCOMPARE(datacube.headers(orientation, 0).size(), kommune_aggregator->categoryCount());
  for (int row = 0; row < datacube.rowCount(); ++row) {
    for (int column = 0; column < datacube.columnCount(); ++column) {
      Q_FOREACH(int cell, datacube.elements(row, column)) {
        ++total;
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, SEX)).toString(), parallel_aggregator->categoryHeaderData(parallel_headers.at(horizontal ? column: row).categoryIndex).toString());
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, KOMMUNE)).toString(), parallel_0_headers.at(horizontal ? column: row));
        QCOMPARE(m_underlying_model->data(m_underlying_model->index(cell, LAST_NAME)).toString(), normal_aggregator->categoryHeaderData(normal_headers.at(horizontal ? row : column).categoryIndex).toString());
      }
    }
  }
  QCOMPARE(total, 100);
}


void testplaincube::test_columnaggregator()
{
  QSharedPointer<ColumnAggregator> first_name_aggregator(new ColumnAggregator(m_underlying_model, testplaincube::FIRST_NAME));
  QCOMPARE(first_name_aggregator->categoryCount(), 14);
  QCOMPARE(first_name_aggregator->categoryHeaderData(0).toString(),QString::fromLocal8Bit("Andersine"));
  QCOMPARE(first_name_aggregator->categoryHeaderData(7).toString(),QString::fromLocal8Bit("Kim"));
  QString baltazar = QString::fromLocal8Bit("Baltazar");
  int cat_of_balt = findCategoryIndexForString(first_name_aggregator, baltazar);
  QCOMPARE(cat_of_balt, 2);
  QCOMPARE((*first_name_aggregator)(0), cat_of_balt);
  QCOMPARE((*first_name_aggregator)(1), cat_of_balt);
  SexAggregator  sex_aggregator(m_underlying_model ,testplaincube::SEX);
  QCOMPARE(sex_aggregator.categoryCount(),2);
  QCOMPARE(sex_aggregator.categoryHeaderData(0).toString(), QString::fromLocal8Bit("female"));
  QCOMPARE(sex_aggregator.categoryHeaderData(1).toString(), QString::fromLocal8Bit("male"));
  QCOMPARE(sex_aggregator.categoryHeaderData(0, Qt::BackgroundRole).value<QColor>().name(), QColor(Qt::magenta).name());
  QCOMPARE(sex_aggregator.categoryHeaderData(1, Qt::BackgroundRole).value<QColor>().name(), QColor(Qt::cyan).name());

}

testplaincube::testplaincube(QObject* parent): danishnamecube_t(parent) {
    load_model_data(QFINDTESTDATA("data/plaincubedata.txt"));
}

void testplaincube::test_autocollapse() {
  Datacube datacube(m_underlying_model, first_name_aggregator, last_name_aggregator);
  datacube.split(Qt::Vertical,1, sex_aggregator);
  // Splitting first names according to sex should yield no extra rows
  QCOMPARE(datacube.rowCount(), first_name_aggregator->categoryCount());
  // While splitting last names should. -1 because there are no
  datacube.split(Qt::Horizontal, 1, sex_aggregator);
  // -1 since there are no female Thomsen in our set
  QCOMPARE(datacube.columnCount(), last_name_aggregator->categoryCount()*sex_aggregator->categoryCount() - 1);

}

void testplaincube::test_filter() {
  Datacube datacube(m_underlying_model, first_name_aggregator, last_name_aggregator);
  int fourty_cat = findCategoryIndexForString(age_aggregator, "40");
  QVERIFY(fourty_cat != -1);

  // Set age filter to include 40-years old only (Expect one result, "Einar Madsen"
  datacube.addFilter(AbstractFilter::Ptr(new FilterByAggregate(age_aggregator, fourty_cat)));
  QCOMPARE(datacube.rowCount(),1);
  QCOMPARE(datacube.columnCount(),1);
  QList<int> rows = datacube.elements(0,0);
  QCOMPARE(rows.size(), 1);
  int row = rows.front();
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, FIRST_NAME)).toString(), QString::fromLocal8Bit("Einar"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, LAST_NAME)).toString(), QString::fromLocal8Bit("Madsen"));
  // Set age filter to include 41-years old only (Expect one result, "Rigmor Jensen", weighting 76
  int fourtyone_cat = findCategoryIndexForString(age_aggregator, "41");
  datacube.resetFilter();
  AbstractFilter::Ptr filter(AbstractFilter::Ptr( new FilterByAggregate(age_aggregator, fourtyone_cat)));
  datacube.addFilter(filter);
  QCOMPARE(datacube.rowCount(),1);
  QCOMPARE(datacube.columnCount(),1);
  rows = datacube.elements(0,0);
  QCOMPARE(rows.size(), 1);
  row = rows.front();
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, FIRST_NAME)).toString(), QString::fromLocal8Bit("Rigmor"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, LAST_NAME)).toString(), QString::fromLocal8Bit("Jensen"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, WEIGHT)).toString(), QString::fromLocal8Bit("76"));
  // Get all with that weight (besides Rigmor Jensen, this includes Lulu Petersen)
  int seventysix = findCategoryIndexForString(weight_aggregator, "76");
  datacube.removeFilter(filter);
  datacube.addFilter(AbstractFilter::Ptr(new FilterByAggregate(weight_aggregator, seventysix)));
  QCOMPARE(datacube.rowCount(),2);
  QCOMPARE(datacube.columnCount(),2);
  rows = datacube.elements(1,0);
  QCOMPARE(rows.size(), 1);
  row = rows.front();
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, FIRST_NAME)).toString(), QString::fromLocal8Bit("Rigmor"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, LAST_NAME)).toString(), QString::fromLocal8Bit("Jensen"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, WEIGHT)).toString(), QString::fromLocal8Bit("76"));
  rows = datacube.elements(0,1);
  QCOMPARE(rows.size(), 1);
  row = rows.front();
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, FIRST_NAME)).toString(), QString::fromLocal8Bit("Lulu"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, LAST_NAME)).toString(), QString::fromLocal8Bit("Petersen"));
  QCOMPARE(m_underlying_model->data(m_underlying_model->index(row, WEIGHT)).toString(), QString::fromLocal8Bit("76"));

}

void testplaincube::test_deep_header() {
  Datacube datacube(m_underlying_model, sex_aggregator, last_name_aggregator);
  datacube.split(Qt::Horizontal, 1, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1,sex_aggregator);
  datacube.split(Qt::Horizontal, 1,age_aggregator);
  datacube.split(Qt::Horizontal, 1,weight_aggregator);
  QCOMPARE(datacube.headerCount(Qt::Horizontal),5);
  for (int headerno = 0; headerno < datacube.headerCount(Qt::Horizontal); ++headerno) {
    int total = 0;
    Q_FOREACH(Datacube::HeaderDescription header_pair, datacube.headers(Qt::Horizontal, headerno)) {
      total += header_pair.span;
    }
    QCOMPARE(total, datacube.columnCount());
  }
}

void testplaincube::test_section_for_element_internal()
{
  Datacube datacube(m_underlying_model, last_name_aggregator, first_name_aggregator);
  datacube.split(Qt::Vertical, 0, age_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);
  datacube.split(Qt::Vertical, 1, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1, kommune_aggregator);
  datacube.collapse(Qt::Vertical, 1);
  datacube.collapse(Qt::Horizontal, 1);
  for (int element=0; element<m_underlying_model->rowCount(); ++element) {
    int row = datacube.internalSection(element, Qt::Vertical);
    int column = datacube.internalSection(element, Qt::Horizontal);
    QVERIFY(datacube.elements(row, column).contains(element));
    QCOMPARE(datacube.sectionForElement(element, Qt::Vertical), row);
    QCOMPARE(datacube.sectionForElement(element, Qt::Horizontal), column);
  }
}

void testplaincube::test_reverse_index()
{
  Datacube datacube(m_underlying_model, last_name_aggregator, first_name_aggregator);
  datacube.split(Qt::Vertical, 0, age_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);
  datacube.split(Qt::Vertical, 1, kommune_aggregator);
  datacube.split(Qt::Horizontal, 1, kommune_aggregator);
  datacube.collapse(Qt::Vertical, 1);
  datacube.collapse(Qt::Horizontal, 1);
  for (int element=0; element<m_underlying_model->rowCount(); ++element) {
    int row = datacube.sectionForElement(element, Qt::Vertical);
    int column = datacube.sectionForElement(element, Qt::Horizontal);
    QVERIFY(datacube.elements(row, column).contains(element));
  }

}

void testplaincube::test_add_category() {
  QStandardItemModel* tmp_model = copy_model();
  QCOMPARE(tmp_model->rowCount(), 100);
  AbstractAggregator::Ptr sex_aggregator(new ColumnAggregator(tmp_model, SEX));
  AbstractAggregator::Ptr kommune_aggregator(new ColumnAggregator(tmp_model, KOMMUNE));
  AbstractAggregator::Ptr first_name_aggregator(new ColumnAggregator(tmp_model, FIRST_NAME));
  AbstractAggregator::Ptr last_name_aggregator(new ColumnAggregator(tmp_model, LAST_NAME));
  Datacube datacube(tmp_model, last_name_aggregator, first_name_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);
  datacube.split(Qt::Horizontal, 2, kommune_aggregator);
  QList<QStandardItem*> row;
  for (int c=0; c<tmp_model->columnCount(); ++c) {
    switch (static_cast<columns_t>(c)) {
      case FIRST_NAME:
        row << new QStandardItem("Agnete");
        break;
      case LAST_NAME:
        row << new QStandardItem("Johansen");
        break;
      case SEX:
        row << new QStandardItem("female");
        break;
      case AGE:
        row << new QStandardItem("20");
        break;
      case WEIGHT:
        row << new QStandardItem("80");
        break;
      case KOMMUNE:
        row << new QStandardItem("Hellerup");
        break;
      case N_COLUMNS:
        Q_ASSERT(false);
    }
  }
    {
        int counter =0;
        for (int r = 0; r < datacube.rowCount(); ++r) {
            for (int c = 0; c < datacube.columnCount(); ++c) {
            QList<int> elements = datacube.elements(r,c);
            counter+=elements.count();
            }
        }
        QCOMPARE(counter,100);
    }
  tmp_model->appendRow(row);
  QVERIFY(findCategoryIndexForString(first_name_aggregator, "Agnete") != -1);
  QCOMPARE(tmp_model->rowCount(), 101);
    {
        int counter =0;
        for (int r = 0; r < datacube.rowCount(); ++r) {
            for (int c = 0; c < datacube.columnCount(); ++c) {
            QList<int> elements = datacube.elements(r,c);
            counter+=elements.count();
            }
        }
        QCOMPARE(counter,101);
    }
  QStringList column0_headers;
  AbstractAggregator::Ptr column0_aggregator = datacube.columnAggregators().at(0);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,0)) {
    for (int i=0; i<hp.span; ++i) {
      column0_headers << column0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList column1_headers;
  AbstractAggregator::Ptr column1_aggregator = datacube.columnAggregators().at(1);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,1)) {
    for (int i=0; i<hp.span; ++i) {
      column1_headers << column1_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList column2_headers;
  AbstractAggregator::Ptr column2_aggregator = datacube.columnAggregators().at(2);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,2)) {
    QCOMPARE(hp.span, 1);
    for (int i=0; i<hp.span; ++i) {
      column2_headers << column2_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  int total = 0;
  for (int r = 0; r < datacube.rowCount(); ++r) {
    for (int c = 0; c < datacube.columnCount(); ++c) {
      QList<int> elements = datacube.elements(r,c);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(tmp_model->data(tmp_model->index(cell, SEX)).toString(), column0_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, FIRST_NAME)).toString(), column1_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, KOMMUNE)).toString(), column2_headers.at(c));
      }
    }
  }
  QCOMPARE(total, 101);

}

void testplaincube::test_add_category_simple()
{
  QStandardItemModel* tmp_model = copy_model();
  AbstractAggregator::Ptr sex_aggregator(new ColumnAggregator(tmp_model, SEX));
  AbstractAggregator::Ptr kommune_aggregator(new ColumnAggregator(tmp_model, KOMMUNE));
  AbstractAggregator::Ptr first_name_aggregator(new ColumnAggregator(tmp_model, FIRST_NAME));
  AbstractAggregator::Ptr last_name_aggregator(new ColumnAggregator(tmp_model, LAST_NAME));
  Datacube datacube(tmp_model, kommune_aggregator, sex_aggregator);
  QList<QStandardItem*> row;
  for (int c=0; c<tmp_model->columnCount(); ++c) {
    switch (static_cast<columns_t>(c)) {
      case FIRST_NAME:
        row << new QStandardItem("Andrea");
        break;
      case LAST_NAME:
        row << new QStandardItem("Hansen");
        break;
      case SEX:
        row << new QStandardItem("neither");
        break;
      case AGE:
        row << new QStandardItem("20");
        break;
      case WEIGHT:
        row << new QStandardItem("80");
        break;
      case KOMMUNE:
        row << new QStandardItem("Hellerup");
        break;
      case N_COLUMNS:
        Q_ASSERT(false);
    }
  }
  tmp_model->appendRow(row);
  QCOMPARE(tmp_model->rowCount(), 101);
  QStringList column0_headers;
  AbstractAggregator::Ptr column0_aggregator = datacube.columnAggregators().at(0);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,0)) {
    for (int i=0; i<hp.span; ++i) {
      column0_headers << column0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  int total = 0;
  for (int r = 0; r < datacube.rowCount(); ++r) {
    for (int c = 0; c < datacube.columnCount(); ++c) {
      QList<int> elements = datacube.elements(r,c);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(tmp_model->data(tmp_model->index(cell, SEX)).toString(), column0_headers.at(c));
      }
    }
  }
  QCOMPARE(total, 101);

}


void testplaincube::test_delete_rows() {
  QStandardItemModel* tmp_model = copy_model();
  AbstractAggregator::Ptr sex_aggregator(new ColumnAggregator(tmp_model, SEX));
  AbstractAggregator::Ptr kommune_aggregator(new ColumnAggregator(tmp_model, KOMMUNE));
  Datacube datacube(tmp_model, kommune_aggregator, sex_aggregator);
  tmp_model->removeRows(30, 10);
  QCOMPARE(m_underlying_model->rowCount()-10, tmp_model->rowCount());
  int total = 0;
  AbstractAggregator::Ptr column0_aggregator = datacube.columnAggregators().at(0);
  QStringList column0_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,0)) {
    for (int i=0; i<hp.span; ++i) {
      column0_headers << column0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  AbstractAggregator::Ptr row0_aggregator = datacube.rowAggregators().at(0);
  QStringList row0_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Vertical,0)) {
    for (int i=0; i<hp.span; ++i) {
      row0_headers << row0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  for (int r = 0; r < datacube.rowCount(); ++r) {
    for (int c = 0; c < datacube.columnCount(); ++c) {
      QList<int> elements = datacube.elements(r,c);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(tmp_model->data(tmp_model->index(cell, SEX)).toString(), column0_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, KOMMUNE)).toString(), row0_headers.at(r));
      }
    }
  }
  QCOMPARE(total, tmp_model->rowCount());

}

void testplaincube::test_add_rows() {
  QStandardItemModel* tmp_model = copy_model();
  AbstractAggregator::Ptr sex_aggregator(new ColumnAggregator(tmp_model, SEX));
  AbstractAggregator::Ptr kommune_aggregator(new ColumnAggregator(tmp_model, KOMMUNE));
  Datacube datacube(tmp_model, kommune_aggregator, sex_aggregator);
  for (int i=0; i<10; ++i) {
    int source_row = i*7+2;
    QList<QStandardItem*> newrow;
    for (int c=0; c<tmp_model->columnCount(); ++c) {
      newrow << new QStandardItem(tmp_model->item(source_row, c)->text());
    }
    int dest_row = i*6+7;
    tmp_model->insertRow(dest_row, newrow);
  }
  QCOMPARE(m_underlying_model->rowCount()+10, tmp_model->rowCount());
  int total = 0;
  QStringList column0_headers;
  AbstractAggregator::Ptr column0_aggregator = datacube.columnAggregators().at(0);
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,0)) {
    for (int i=0; i<hp.span; ++i) {
      column0_headers << column0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  AbstractAggregator::Ptr row0_aggregator = datacube.rowAggregators().at(0);
  QStringList row0_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Vertical,0)) {
    for (int i=0; i<hp.span; ++i) {
      row0_headers << row0_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  for (int r = 0; r < datacube.rowCount(); ++r) {
    for (int c = 0; c < datacube.columnCount(); ++c) {
      QList<int> elements = datacube.elements(r,c);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(tmp_model->data(tmp_model->index(cell, SEX)).toString(), column0_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, KOMMUNE)).toString(), row0_headers.at(r));
      }
    }
  }
  QCOMPARE(total, tmp_model->rowCount());
}

void testplaincube::test_remove_category()
{
  QStandardItemModel* tmp_model = copy_model();
  QSharedPointer<ColumnAggregator> age_aggregator(new ColumnAggregator(tmp_model, AGE));
  AbstractAggregator::Ptr kommune_aggregator(new ColumnAggregator(tmp_model, KOMMUNE));
  AbstractAggregator::Ptr sex_aggregator(new ColumnAggregator(tmp_model, SEX));
  AbstractAggregator::Ptr last_name_aggregator(new ColumnAggregator(tmp_model, LAST_NAME));
  Datacube datacube(tmp_model, kommune_aggregator, age_aggregator);
  datacube.split(Qt::Horizontal, 1, last_name_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);
  QList<int> aged20;
  aged20 << 65 << 32 << 12; // Inverse order to help removal
  Q_FOREACH(int row, aged20) {
    QCOMPARE(tmp_model->index(row, AGE).data().toInt(), 20);
  }
  QVERIFY(findCategoryIndexForString(age_aggregator, "20") != -1);

  Q_FOREACH(int row, aged20) {
    tmp_model->removeRow(row);
  }
  // At this point, we should not see the removal triggered
  QVERIFY(findCategoryIndexForString(age_aggregator,"20") != -1);

  // Manually trigger removal
  age_aggregator->resetCategories();

  // age_filter should now not have an "20" aged category
  QVERIFY(findCategoryIndexForString(age_aggregator, "20") == -1);

  // Check datacube
  int total = 0;
  QStringList column0_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,0)) {
    for (int i=0; i<hp.span; ++i) {
      column0_headers << sex_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList column1_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,1)) {
    for (int i=0; i<hp.span; ++i) {
      column1_headers << age_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList column2_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Horizontal,2)) {
    for (int i=0; i<hp.span; ++i) {
      column2_headers << last_name_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  QStringList row0_headers;
  Q_FOREACH(Datacube::HeaderDescription hp, datacube.headers(Qt::Vertical,0)) {
    for (int i=0; i<hp.span; ++i) {
      row0_headers << kommune_aggregator->categoryHeaderData(hp.categoryIndex).toString();
    }
  }
  for (int r = 0; r < datacube.rowCount(); ++r) {
    for (int c = 0; c < datacube.columnCount(); ++c) {
      QList<int> elements = datacube.elements(r,c);
      Q_FOREACH(int cell, elements) {
        ++total;
        QCOMPARE(tmp_model->data(tmp_model->index(cell, SEX)).toString(), column0_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, AGE)).toString(), column1_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, LAST_NAME)).toString(), column2_headers.at(c));
        QCOMPARE(tmp_model->data(tmp_model->index(cell, KOMMUNE)).toString(), row0_headers.at(r));
      }
    }
  }
  QCOMPARE(total, 97);
}

void testplaincube::test_header_element_count()
{
  Datacube datacube(m_underlying_model, last_name_aggregator, first_name_aggregator);
  datacube.split(Qt::Vertical, 1, age_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);

  // Go through rows, do the sums manually and test versus the number the datacube provides
  {
    QList<Datacube::HeaderDescription> row0_headers = datacube.headers(Qt::Vertical,0);
    QList<Datacube::HeaderDescription> row1_headers = datacube.headers(Qt::Vertical,1);
    int h1 = 0;
    for (int h0=0; h0 < row0_headers.size(); ++h0) {
      Datacube::HeaderDescription row0_header = row0_headers.at(h0);
      const int row0_section = row0_header.categoryIndex;
      for (int h1_end = h1+row0_header.span; h1 < h1_end; ++h1) {
        Datacube::HeaderDescription row1_header = row1_headers.at(h1);
        Q_ASSERT(row1_header.span == 1); // If we split futher, we must be more clever)
        const int row1_section = row1_header.categoryIndex;
        QList<int> row1_elements = datacube.elements(Qt::Vertical, 1, h1);
        int count = 0;
        for (int i=0; i<m_underlying_model->rowCount(); ++i) {
          if ((*age_aggregator)(i) == row1_section && (*last_name_aggregator)(i) == row0_section) {
            ++count;
            QVERIFY(row1_elements.contains(i)); // Check that elements does contain the selected element
          }
        }
        QVERIFY(count>0); // Otherwise, they should have collapsed.
        QCOMPARE(datacube.elementCount(Qt::Vertical, 1, h1), row1_elements.size());
        QCOMPARE(datacube.elementCount(Qt::Vertical, 1, h1), count);
      }
      QList<int> row0_elements = datacube.elements(Qt::Vertical, 0, h0);
      int count = 0;
      for (int i=0; i<m_underlying_model->rowCount(); ++i) {
        if ((*last_name_aggregator)(i) == row0_section) {
          QVERIFY(row0_elements.contains(i));
          ++count;
        }
      }
      QCOMPARE(datacube.elementCount(Qt::Vertical, 0, h0), row0_elements.size());
      QCOMPARE(datacube.elementCount(Qt::Vertical, 0, h0), count);
    }
  }

  // Do the same with the columns
  {
    QList<Datacube::HeaderDescription> col0_headers = datacube.headers(Qt::Horizontal,0);
    QList<Datacube::HeaderDescription> col1_headers = datacube.headers(Qt::Horizontal,1);
    int h1 = 0;
    for (int h0=0; h0 < col0_headers.size(); ++h0) {
      Datacube::HeaderDescription col0_header = col0_headers.at(h0);
      const int col0_section = col0_header.categoryIndex;
      for (int h1_end = h1+col0_header.span;h1 < h1_end; ++h1) {
        Datacube::HeaderDescription col1_header = col1_headers.at(h1);
        Q_ASSERT(col1_header.span == 1); // If we split futher, we must be more clever)
        const int col1_section = col1_header.categoryIndex;
        QList<int> column1_elements = datacube.elements(Qt::Horizontal,1,h1);
        int count = 0;
        for (int i=0; i<m_underlying_model->rowCount(); ++i) {
          if ((*first_name_aggregator)(i) == col1_section && (*sex_aggregator)(i) == col0_section) {
            ++count;
            QVERIFY(column1_elements.contains(i));
          }
        }
        QVERIFY(count>0); // Otherwise, they should have collapsed.
        QCOMPARE(datacube.elementCount(Qt::Horizontal, 1, h1), column1_elements.size());
        QCOMPARE(datacube.elementCount(Qt::Horizontal, 1, h1), count);
      }
      int count = 0;
        QList<int> column0_elements = datacube.elements(Qt::Horizontal,0,h0);
      for (int i=0; i<m_underlying_model->rowCount(); ++i) {
        if ((*sex_aggregator)(i) == col0_section) {
          ++count;
            QVERIFY(column0_elements.contains(i));
        }
      }
      QCOMPARE(datacube.elementCount(Qt::Horizontal, 0, h0), column0_elements.size());
      QCOMPARE(datacube.elementCount(Qt::Horizontal, 0, h0), count);
    }
  }
}

// Make QTest able to print QPair<int,int>
namespace QTest {
template<>
char* toString(const QPair<int,int>& value)
{
  QByteArray ba = "(";
  ba += QByteArray::number(value.first) + ", " + QByteArray::number(value.second);
  ba += ")";
  return qstrdup(ba.data());
}
} // End of namespace QTest


void testplaincube::test_section_to_header_section_and_back_again()
{
  Datacube datacube(m_underlying_model, last_name_aggregator, first_name_aggregator);
  datacube.split(Qt::Vertical, 1, age_aggregator);
  datacube.split(Qt::Horizontal, 0, sex_aggregator);

  // Go through rows, testing that to_section and to_header_section returns correspond with manual counts
  {
    QList<Datacube::HeaderDescription> row0_headers = datacube.headers(Qt::Vertical,0);
    QList<Datacube::HeaderDescription> row1_headers = datacube.headers(Qt::Vertical,1);
    int h1 = 0;
    for (int h0=0; h0 < row0_headers.size(); ++h0) {
      Datacube::HeaderDescription row0_header = row0_headers.at(h0);
      typedef QPair<int,int> intpair_t;
      QCOMPARE(datacube.toSection(Qt::Vertical, 0, h0), intpair_t(h1,h1+row0_header.span-1));
      for (int h1_end = h1+row0_header.span;h1 < h1_end; ++h1) {
        Datacube::HeaderDescription row1_header = row1_headers.at(h1);
        Q_UNUSED(row1_header)
        Q_ASSERT(row1_header.span == 1); // If we split futher, we must be more clever)
        QCOMPARE(datacube.toHeaderSection(Qt::Vertical, 1, h1), h1);
        QCOMPARE(datacube.toHeaderSection(Qt::Vertical, 0, h1), h0);
        QCOMPARE(datacube.toSection(Qt::Vertical, 1, h1), intpair_t(h1,h1));
      }
    }
  }
  // Go through columns, testing that to_section and to_header_section returns correspond with manual counts
  {
    QList<Datacube::HeaderDescription> column0_headers = datacube.headers(Qt::Horizontal,0);
    QList<Datacube::HeaderDescription> column1_headers = datacube.headers(Qt::Horizontal,1);
    int h1 = 0;
    for (int h0=0; h0 < column0_headers.size(); ++h0) {
      Datacube::HeaderDescription row0_header = column0_headers.at(h0);
      typedef QPair<int,int> intpair_t;
      QCOMPARE(datacube.toSection(Qt::Horizontal, 0, h0), intpair_t(h1,h1+row0_header.span-1));
      for (int h1_end = h1+row0_header.span ;h1 < h1_end; ++h1) {
        Datacube::HeaderDescription row1_header = column1_headers.at(h1);
        Q_UNUSED(row1_header)
        Q_ASSERT(row1_header.span == 1); // If we split futher, we must be more clever)
        QCOMPARE(datacube.toHeaderSection(Qt::Horizontal, 1, h1), h1);
        QCOMPARE(datacube.toHeaderSection(Qt::Horizontal, 0, h1), h0);
        QCOMPARE(datacube.toSection(Qt::Horizontal, 1, h1), intpair_t(h1,h1));
      }
    }
  }
}

class TenBucketsNonAggregator : public AbstractAggregator {
        QList<QString> cats;
    public:
        explicit TenBucketsNonAggregator(QAbstractItemModel* model) : AbstractAggregator(model) {
            for(int i = 0 ; i < 10 ; i++) {
                cats << QString::number(i);
            }
        }

        virtual int categoryCount() const {
            return cats.size();
        }

        virtual QVariant categoryHeaderData(int category, int role = Qt::DisplayRole) const {
            if(role == Qt::DisplayRole) {
                return cats.at(category);
            }
            return QVariant();
        }

        virtual int operator()(int row) const{
            Q_UNUSED(row);
            return 0;
        }
};

void testplaincube::test_many_buckets()
{
    try {
        {
            Datacube datacube(m_underlying_model, AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)), AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)));
            for(int i = 0 ; i <7 ; i++) {
                datacube.split(Qt::Horizontal,0, AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)));
                QCOMPARE(datacube.headerCount(Qt::Horizontal),i+2);
                datacube.split(Qt::Vertical,0,AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)));
                QCOMPARE(datacube.headerCount(Qt::Vertical),i+2);
           }
           //due to overflow-guards, these shouldn't actually succeed.
           int horizontal_header_count = datacube.headerCount(Qt::Horizontal);
           datacube.split(Qt::Horizontal,0, AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)));
           QCOMPARE(horizontal_header_count,datacube.headerCount(Qt::Horizontal));
           int vertical_header_count = datacube.headerCount(Qt::Vertical);
           datacube.split(Qt::Vertical,0, AbstractAggregator::Ptr(new TenBucketsNonAggregator(m_underlying_model)));
           QCOMPARE(vertical_header_count,datacube.headerCount(Qt::Vertical));
        }
    } catch (std::bad_alloc& ex) {
        QFAIL("Should suceeed");
    }
}

QTEST_GUILESS_MAIN(testplaincube)

#include "testplaincube.moc"
