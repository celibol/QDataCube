#include "abstractaggregator.h"

class qdatacube::AbstractAggregatorPrivate {
    public:
        AbstractAggregatorPrivate(const QAbstractItemModel* underlying_model) : m_underlying_model(underlying_model), m_name("unnamed") {
        }
        const QAbstractItemModel* m_underlying_model;
        QString m_name;
};

qdatacube::AbstractAggregator::AbstractAggregator(const QAbstractItemModel* model):
    d(new AbstractAggregatorPrivate(model))
{
  Q_ASSERT(model);
}

QString qdatacube::AbstractAggregator::name() const {
    return d->m_name;
}

const QAbstractItemModel* qdatacube::AbstractAggregator::underlyingModel() const {
    return d->m_underlying_model;
}

qdatacube::AbstractAggregator::~AbstractAggregator() {

}

void qdatacube::AbstractAggregator::setName(const QString& newName) {
    d->m_name = newName;
}


#include "abstractaggregator.moc"
