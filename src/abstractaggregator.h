/*
 Author: Ange Optimization <esben@ange.dk>  (C) Ange Optimization ApS 2009

 Copyright: See COPYING file that comes with this distribution

*/

#ifndef QDATACUBE_ABSTRACT_AGGREGATOR_H
#define QDATACUBE_ABSTRACT_AGGREGATOR_H
#include <QList>

#include "qdatacube_export.h"
#include <QString>
#include <QObject>
#include <QVariant>

template<class T >
class QSharedPointer;
class QAbstractItemModel;

namespace qdatacube {

/**
 * aggregate elements into a number of categories
 */
class AbstractAggregatorPrivate;
class QDATACUBE_EXPORT AbstractAggregator : public QObject {
    Q_OBJECT
    public:
        typedef QSharedPointer<AbstractAggregator> Ptr;
        explicit AbstractAggregator(const QAbstractItemModel* model);

        /**
         * @param row number in m_model
         * @returns the category number for row, 0 <= return value < categories().size()
         *
         */
        virtual int operator()(int row) const = 0;

        /**
         * @return the number of categories in this aggregator
         */
        virtual int categoryCount() const = 0;

        /**
         * @param  category to query
         * @param role header data role to query
         * @return the headerdata for a category and a role.
         */
        virtual QVariant categoryHeaderData(int category, int role = Qt::DisplayRole) const = 0;

        /**
         * @returns an name for this aggregator. Default implementation returns "unnamed";
         */
        QString name() const;

        /**
        * @return underlying model
        */
        const QAbstractItemModel* underlyingModel() const;

        /**
        * dtor
        */
        virtual ~AbstractAggregator();
    Q_SIGNALS:
        /**
         * Implementors must emit this signal when a category has been added
         * @param index index of removed category
         */
        void categoryAdded(int index) const;

        /**
         * Implementors must emit this signal when a category has been removed
         * @param index index of new category
         */
        void categoryRemoved(int index) const;

    protected:
        /**
         * Sets the name of this aggregator to \param newName
         */
        void setName(const QString& newName);

    private:
        QScopedPointer<AbstractAggregatorPrivate> d;
};

}

#endif // QDATACUBE_ABSTRACT_AGGREGATOR_H
