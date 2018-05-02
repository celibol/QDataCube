#include "cell.h"

namespace qdatacube {

QDebug operator<<(QDebug dbg, const Cell& cell)
{
  dbg.nospace() << "(" << cell.row() << "," << cell.column() << ")";
  return dbg.space();
}


} // end of namespace

