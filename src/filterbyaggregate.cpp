#include "filterbyaggregate.h"

#include "abstractaggregator.h"

#include <QSharedPointer>

namespace qdatacube {

class FilterByAggregatePrivate {
public:
    FilterByAggregatePrivate(FilterByAggregate* q, AbstractAggregator::Ptr aggregator, const QString& category,
                             int categoryIndex);
    AbstractAggregator::Ptr m_aggregator;
    QString m_category;
    int m_categoryIndex;
};

FilterByAggregatePrivate::FilterByAggregatePrivate(FilterByAggregate* q, AbstractAggregator::Ptr aggregator,
                                                   const QString& category, int categoryIndex)
  : m_aggregator(aggregator), m_category(category), m_categoryIndex(categoryIndex)
{
    Q_ASSERT(aggregator);
    Q_ASSERT(categoryIndex < aggregator->categoryCount());
    QObject::connect(aggregator.data(), &AbstractAggregator::categoryAdded, q, &FilterByAggregate::slot_aggregator_category_inserted);
    QObject::connect(aggregator.data(), &AbstractAggregator::categoryRemoved, q, &FilterByAggregate::slot_aggregator_category_removed);
    q->setShortName(m_category);
    q->setName(m_aggregator->name() + "=" + m_category);
}

// Utility function to find the index corresponding to a category
static int categoryToIndex(const AbstractAggregator::Ptr aggregator, const QString& category) {
    int categoryIndex = -1;
    for (int i = 0 ; i < aggregator->categoryCount(); i += 1) {
        if (aggregator->categoryHeaderData(i, Qt::DisplayRole).toString() == category) {
            categoryIndex = i;
            break;
        }
    }
    return categoryIndex;
}

FilterByAggregate::FilterByAggregate(AbstractAggregator::Ptr aggregator, int categoryIndex)
  : AbstractFilter(aggregator->underlyingModel()),
    d(new FilterByAggregatePrivate(this, aggregator, aggregator->categoryHeaderData(categoryIndex).toString(), categoryIndex))
{
    Q_ASSERT(0 <= categoryIndex);
}

FilterByAggregate::FilterByAggregate(AbstractAggregator::Ptr aggregator, const QString& category)
  : AbstractFilter(aggregator->underlyingModel()),
    d(new FilterByAggregatePrivate(this, aggregator, category, categoryToIndex(aggregator, category)))
{
    // Empty
}

FilterByAggregate::~FilterByAggregate() {
    // Empty
}

bool FilterByAggregate::operator()(int row) const {
    return d->m_aggregator->operator()(row) == d->m_categoryIndex;
}

void FilterByAggregate::slot_aggregator_category_inserted(int index) {
    if (d->m_categoryIndex == -1) {
        d->m_categoryIndex = categoryToIndex(d->m_aggregator, d->m_category);
    } else if (index <= d->m_categoryIndex) {
        d->m_categoryIndex += 1;
    }
}

void FilterByAggregate::slot_aggregator_category_removed(int index) {
    if (index == d->m_categoryIndex) {
        d->m_categoryIndex = -1;
    } else if (index < d->m_categoryIndex) {
        d->m_categoryIndex += -1;
    }
}

AbstractAggregator::Ptr FilterByAggregate::aggregator() const {
    return d->m_aggregator;
}

int FilterByAggregate::categoryIndex() const {
    return d->m_categoryIndex;
}

} // namespace qdatacube

#include "filterbyaggregate.moc"
