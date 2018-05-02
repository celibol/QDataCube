#include "danishnamecube.h"
#include "datacube.h"
#include "filterbyaggregate.h"

#include <QObject>
#include <QSharedPointer>
#include <QStandardItemModel>
#include <QTest>

using namespace qdatacube;

class TestDatacube : public QObject {
    Q_OBJECT
private Q_SLOTS:

    void testFilterByAggregate();
};
QTEST_GUILESS_MAIN(TestDatacube)

void TestDatacube::testFilterByAggregate() {
    danishnamecube_t danishModelHolder;
    danishModelHolder.load_model_data(QFINDTESTDATA("data/plaincubedata.txt"));
    Datacube datacube(danishModelHolder.m_underlying_model);
    QCOMPARE(datacube.elementCount(), 100);

    AbstractFilter::Ptr maleFilter(new FilterByAggregate(danishModelHolder.sex_aggregator, "male"));
    datacube.addFilter(maleFilter);
    QCOMPARE(datacube.elementCount(), 58);

    datacube.removeFilter(maleFilter);
    QCOMPARE(datacube.elementCount(), 100);

    QSharedPointer<FilterByAggregate> otherFilter(new FilterByAggregate(danishModelHolder.sex_aggregator, "other"));
    datacube.addFilter(otherFilter);
    QCOMPARE(datacube.elementCount(), 0);
    QCOMPARE(danishModelHolder.sex_aggregator->categoryCount(), 2);
    QCOMPARE(otherFilter->categoryIndex(), -1);

    QList<QStandardItem*> otherRow;
    otherRow << new QStandardItem() << new QStandardItem() << new QStandardItem("other"); // 3 columns ignored
    danishModelHolder.m_underlying_model->appendRow(otherRow);
    QCOMPARE(datacube.elementCount(), 1);
    QCOMPARE(danishModelHolder.sex_aggregator->categoryCount(), 3);
    QCOMPARE(otherFilter->categoryIndex(), 2);

    danishModelHolder.m_underlying_model->removeRow(100);
    QCOMPARE(datacube.elementCount(), 0);
    QEXPECT_FAIL(0, "ColumnAggregator is collapsing lazily", Continue);
    QCOMPARE(danishModelHolder.sex_aggregator->categoryCount(), 2);
    QEXPECT_FAIL(0, "ColumnAggregator is collapsing lazily", Continue);
    QCOMPARE(otherFilter->categoryIndex(), -1);

    // When forcing ColumnAggregator to collapse the tests passes
    danishModelHolder.sex_aggregator->resetCategories();
    QCOMPARE(danishModelHolder.sex_aggregator->categoryCount(), 2);
    QCOMPARE(otherFilter->categoryIndex(), -1);
}

#include "testdatacube.moc"
