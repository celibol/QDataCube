/*
 Author: Ange Optimization <contact@ange.dk>  (C) Ange Optimization ApS 2010

 Copyright: See COPYING file that comes with this distribution

*/

#include "datacubeview.h"
#include <QMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include "datacube.h"
#include "datacubeselection.h"
#include "cell.h"
#include <QScrollBar>
#include <QToolTip>
#include "abstractaggregator.h"
#include "abstractfilter.h"
#include "abstractformatter.h"

#include "datacubeview_p.h"

#include <QSharedPointer>


/**
 * Small RAII struct to help save/restore painter state
 */
struct PainterSaver {
    public:
        PainterSaver(QPainter* painter) : m_painter(painter) {
            m_painter->save();
        }
        ~PainterSaver() {
            m_painter->restore();
        }
    private:
        QPainter* m_painter;
};

namespace qdatacube {

DatacubeViewPrivate::DatacubeViewPrivate(DatacubeView* datacubeview)
    : q(datacubeview),
    datacube(0L),
    selection(0L),
    horizontal_header_height(-1),
    vertical_header_width(-1),
    cell_size(),
    datacube_size(),
    show_totals(true)
{

}

Cell DatacubeViewPrivate::cell_for_position(QPoint pos, int vertical_scrollbar_value, int horizontal_scrollbar_value) const {
  if(!datacube) {
    return Cell();
  }
  const int ncolumn_headers = datacube->headerCount(Qt::Horizontal);
  const int nrow_headers = datacube->headerCount(Qt::Vertical);
  const int column = (pos.x() / cell_size.width()) - nrow_headers;
  const int row = (pos.y() / cell_size.height()) - ncolumn_headers;
  const int nrows = datacube_size.height() + (show_totals ? datacube->headerCount(Qt::Horizontal) : 0);
  const int ncolumns = datacube_size.width() + (show_totals ? datacube->headerCount(Qt::Vertical) : 0);
  if (0 <= row && 0 <= column && nrows && column < ncolumns && (row <= datacube_size.height() || column < datacube_size.width())) {
    return Cell(row + vertical_scrollbar_value, column + horizontal_scrollbar_value);
  } else if (row < 0 && -row <= datacube->headerCount(Qt::Horizontal)) {
    if (column>=0 && column < ncolumns) {
      // row headers
      return Cell(row, column + horizontal_scrollbar_value);
    } else if (column < 0 && -column <= datacube->headerCount(Qt::Vertical)) {
      return Cell(row, column);
    }
  } else if (column < 0 && -column <= datacube->headerCount(Qt::Vertical) && row>=0 && row < nrows) {
    return Cell(row + vertical_scrollbar_value, column);
  }
  return Cell();
}


DatacubeView::DatacubeView(QWidget* parent):
    QAbstractScrollArea(parent),
    d(new DatacubeViewPrivate(this)) {
}

void DatacubeView::setDatacube(Datacube* datacube) {
  if (d->datacube) {
    d->datacube->disconnect(this);
    d->datacube->disconnect(d.data());
    if (datacube && datacube->underlyingModel() != d->datacube->underlyingModel()) {
      qDeleteAll(d->formatters);
      d->formatters.clear();
    }
  }
  d->datacube = datacube;
  delete d->selection;
  d->selection = new DatacubeSelection(datacube, this);
  viewport()->update();
  connect(d->selection, SIGNAL(selectionStatusChanged(int, int)), viewport(), SLOT(update()));
  connect(datacube, SIGNAL(destroyed(QObject*)), d.data(), SLOT(datacube_deleted()));
  connect(datacube, SIGNAL(reset()), d.data(), SLOT(relayout()));
  connect(datacube, SIGNAL(dataChanged(int, int)), viewport(), SLOT(update()));
  connect(datacube, SIGNAL(columnsInserted(int, int)), d.data(), SLOT(relayout()));
  connect(datacube, SIGNAL(rowsInserted(int, int)), d.data(), SLOT(relayout()));
  connect(datacube, SIGNAL(columnsRemoved(int, int)), d.data(), SLOT(relayout()));
  connect(datacube, SIGNAL(rowsRemoved(int, int)), d.data(), SLOT(relayout()));
  connect(datacube, SIGNAL(filterChanged()), d.data(), SLOT(relayout()));
  d->relayout();
}

void DatacubeViewPrivate::relayout() {
  if (!datacube) {
    return; // defer layout to datacube is set
  }
  datacube_size = QSize(datacube->columnCount(), datacube->rowCount());
  QSize new_cell_size(q->fontMetrics().width("9999"), 0);
  Q_FOREACH(AbstractFormatter* formatter, formatters) {
    QSize formatter_cell_size = formatter->cellSize();
    new_cell_size.setWidth(qMax(formatter_cell_size.width()+2, new_cell_size.width()));
    new_cell_size.setHeight(new_cell_size.height() + formatter_cell_size.height());
  }
  if (new_cell_size.height() == 0) {
    new_cell_size.setHeight(10);
  }
  cell_size = new_cell_size;
  vertical_header_width = qMax(1, datacube->headerCount(Qt::Vertical)) * cell_size.width();
  horizontal_header_height = qMax(1, datacube->headerCount(Qt::Horizontal)) * cell_size.height();
  QSize visible_size = q->viewport()->size();
  // Calculate the number of rows and columns at least partly visible
  const int rows_visible = (visible_size.height() - horizontal_header_height + 1) / (cell_size.height());
  const int columns_visible = (visible_size.width() - vertical_header_width + 1) / (cell_size.width());
  const int nrows = datacube_size.height() + (show_totals ? qMax(1,datacube->headerCount(Qt::Horizontal)) : 0);
  const int ncolumns = datacube_size.width() + (show_totals ? qMax(1,datacube->headerCount(Qt::Vertical)) : 0);
  visible_cells = QSize(qMin(columns_visible, ncolumns), qMin(rows_visible, nrows));
  // Set range of scrollbars
  q->verticalScrollBar()->setRange(0, qMax(0, nrows - rows_visible));
  q->verticalScrollBar()->setPageStep(rows_visible);
  q->horizontalScrollBar()->setRange(0, qMax(0, ncolumns - columns_visible));
  q->horizontalScrollBar()->setPageStep(columns_visible);
  q->viewport()->update();
}

void DatacubeView::resizeEvent(QResizeEvent* event)
{
  QAbstractScrollArea::resizeEvent(event);
  d->relayout();
}

DatacubeView::~DatacubeView() {

}

bool DatacubeView::viewportEvent(QEvent* event) {
  if (event->type() == QEvent::Paint) {
    QPaintEvent* paintevent = static_cast<QPaintEvent*>(event);
    d->paint_datacube(paintevent);
  }
  return QAbstractScrollArea::viewportEvent(event);
}

void DatacubeViewPrivate::paint_datacube(QPaintEvent* event) const {
  if(!datacube) {
    return;
  }
  QPainter painter(q->viewport());
  QStyleOption options;
  options.initFrom(q->viewport());
  options.rect.setSize(cell_size);
  QRect header_rect(options.rect);
  painter.setBrush(q->palette().button());
  painter.setPen(q->palette().color(QPalette::WindowText));

  // Draw filter corner, if applicable
  QRect cornerRect(options.rect.topLeft(), QSize(vertical_header_width,horizontal_header_height));
  painter.drawRect(cornerRect);
  painter.setPen(q->palette().buttonText().color());
    if (!datacube->filters().isEmpty()) {
        QString firstFilterCategory;
        if(datacube->filters().size() == 1) {
            firstFilterCategory = datacube->filters().first()->shortName();
        } else {
            firstFilterCategory = QStringLiteral("+++");
        }
        painter.drawText(cornerRect.adjusted(1, 1, -1, -1), Qt::AlignCenter, firstFilterCategory);
    }


  // Draw horizontal header
  const int leftmost_column = q->horizontalScrollBar()->value();
  const int rightmost_column = leftmost_column + visible_cells.width();
  const int topmost_row = q->verticalScrollBar()->value();
  const int bottommost_row = topmost_row + visible_cells.height();
  const int horizontal_header_count = datacube->headerCount(Qt::Horizontal);
  const int ndatarows = datacube->rowCount();
  QRect summary_rect(header_rect);
  summary_rect.translate(0, cell_size.height() * (ndatarows - topmost_row+horizontal_header_count*2-1));
  for (int hh = 0; hh < horizontal_header_count; ++hh) {
    header_rect.moveLeft(q->viewport()->rect().left() + vertical_header_width);
    summary_rect.moveLeft(header_rect.left());
    QList<Datacube::HeaderDescription > headers = datacube->headers(Qt::Horizontal, hh);
    AbstractAggregator::Ptr aggregator = datacube->columnAggregators().at(hh);
    int current_cell_equivalent = 0;
    for (int header_index = 0; header_index < headers.size() && current_cell_equivalent <= rightmost_column; ++header_index) {
        Datacube::HeaderDescription header = headers.at(header_index);
        PainterSaver saver(&painter);
        QVariant maybebackground = aggregator->categoryHeaderData(header.categoryIndex,Qt::BackgroundRole);;
        if(maybebackground.canConvert<QColor>()) {
            painter.setBrush(maybebackground.value<QColor>());
        } else if(maybebackground.canConvert<QBrush>()) {
            painter.setBrush(maybebackground.value<QBrush>());
        } else {
            painter.setBrush((header_selection_area.contains(header_index, -hh-1)) ? q->palette().highlight() : q->palette().button());
        }
      int header_span = header.span;
      current_cell_equivalent += header_span;
      if (current_cell_equivalent < leftmost_column) {
        continue;
      }
      if (current_cell_equivalent > rightmost_column) {
        header_span -= (current_cell_equivalent - rightmost_column - 1);
      }
      if (current_cell_equivalent - header_span < leftmost_column) {
        // Force header cell to available width, for maximum readability
        header_span = (current_cell_equivalent - leftmost_column);
      }
      header_rect.setSize(QSize(cell_size.width()*header_span, cell_size.height()));
      painter.drawRect(header_rect);
        painter.save();
        QVariant maybeforeground = aggregator->categoryHeaderData(header.categoryIndex, Qt::ForegroundRole);
        if(maybeforeground.canConvert<QColor>()) {
            painter.setPen(maybeforeground.value<QColor>());
        } else if(maybebackground.canConvert<QPen>()) {
            painter.setPen(maybeforeground.value<QPen>());
        }
        painter.drawText(header_rect.adjusted(0, 0, 0, 2), Qt::AlignCenter, aggregator->categoryHeaderData(header.categoryIndex).toString());
        painter.restore();
      header_rect.translate(header_rect.width(), 0);
      if (show_totals && bottommost_row >= hh + ndatarows) {
        summary_rect.setSize(header_rect.size());
        painter.drawRect(summary_rect);
        QRect text_rect(summary_rect);
        QList<int> elements = datacube->elements(Qt::Horizontal, hh, header_index);
        Q_FOREACH(AbstractFormatter* formatter, formatters) {
          text_rect.setHeight(formatter->cellSize().height());
          const QString value = formatter->format(elements);
            painter.save();
            QVariant maybeforeground = aggregator->categoryHeaderData(header.categoryIndex, Qt::ForegroundRole);
            if(maybeforeground.canConvert<QColor>()) {
                painter.setPen(maybeforeground.value<QColor>());
            } else if(maybebackground.canConvert<QPen>()) {
                painter.setPen(maybeforeground.value<QPen>());
            }
            painter.drawText(text_rect.adjusted(0,0,0,2), Qt::AlignCenter, value);
            painter.restore();
            text_rect.translate(0, text_rect.height());
        }
        summary_rect.translate(summary_rect.width(), 0);
      }
    }
    header_rect.translate(0, cell_size.height());
    summary_rect.translate(0, -cell_size.height());
  }
  if (horizontal_header_count == 0) {
    header_rect.moveLeft(q->viewport()->rect().left() + vertical_header_width);
    header_rect.setSize(QSize(cell_size.width(), cell_size.height()));
    painter.drawRect(header_rect);
    if(show_totals) {
        summary_rect.moveLeft(q->viewport()->rect().left() + vertical_header_width);
        summary_rect.setSize(QSize(cell_size.width(), cell_size.height()));
        summary_rect.translate(0, cell_size.height()*2);
        painter.drawRect(summary_rect);
    }
  }

  // Draw vertical header
  const int ndatacolumns = datacube->columnCount();
  const int vertical_header_count = datacube->headerCount(Qt::Vertical);
  header_rect.moveLeft(q->viewport()->rect().left());
  summary_rect.moveLeft(header_rect.left() + cell_size.width()*(ndatacolumns-leftmost_column+vertical_header_count*2-1));
  options.rect.moveTop(q->viewport()->rect().top() + horizontal_header_height);

  for (int vh = 0; vh < vertical_header_count; ++vh) {
    header_rect.moveTop(options.rect.top());
    summary_rect.moveTop(header_rect.top());
    QList<Datacube::HeaderDescription > headers = datacube->headers(Qt::Vertical, vh);
    AbstractAggregator::Ptr aggregator = datacube->rowAggregators().at(vh);
    int current_cell_equivalent = 0;
    for (int header_index = 0; header_index < headers.size() && current_cell_equivalent <= bottommost_row; ++header_index) {
        Datacube::HeaderDescription header = headers.at(header_index);
        PainterSaver saver(&painter);
        QVariant maybebackground = aggregator->categoryHeaderData(header.categoryIndex,Qt::BackgroundRole);
        if(maybebackground.canConvert<QColor>()) {
            painter.setBrush(maybebackground.value<QColor>());
        } else if(maybebackground.canConvert<QBrush>()) {
            painter.setBrush(maybebackground.value<QBrush>());
        } else {
            painter.setBrush((header_selection_area.contains(header_index, -vh-1)) ? q->palette().highlight() : q->palette().button());
        }
      int header_span = header.span;
      current_cell_equivalent += header_span;
      if (current_cell_equivalent < topmost_row) {
        continue; // Outside viewport
      }
      if (current_cell_equivalent - header_span < topmost_row) {
        // Force header cell to available width, for maximum readability
        header_span = (current_cell_equivalent - topmost_row);
      }
      if (current_cell_equivalent > bottommost_row) {
        header_span -= (current_cell_equivalent - bottommost_row - 1);
      }
      header_rect.setSize(QSize(cell_size.width(), cell_size.height()*header_span));
      painter.drawRect(header_rect);
        painter.save();
        QVariant maybeforeground = aggregator->categoryHeaderData(header.categoryIndex, Qt::ForegroundRole);
        if(maybeforeground.canConvert<QColor>()) {
            painter.setPen(maybeforeground.value<QColor>());
        } else if(maybebackground.canConvert<QPen>()) {
            painter.setPen(maybeforeground.value<QPen>());
        }
        painter.drawText(header_rect.adjusted(0, 0, 0, 2), Qt::AlignCenter, aggregator->categoryHeaderData(header.categoryIndex).toString());
      painter.restore();
      header_rect.translate(0, header_rect.height());
      if (show_totals && rightmost_column >= vh + ndatacolumns) {
        summary_rect.setSize(header_rect.size());
        painter.drawRect(summary_rect);
        QRect text_rect(summary_rect);
        text_rect.translate(0, (summary_rect.height()-cell_size.height())/2); // Center vertically
        QList<int> elements = datacube->elements(Qt::Vertical, vh, header_index);
        Q_FOREACH(AbstractFormatter* formatter, formatters) {
          text_rect.setHeight(formatter->cellSize().height());
          const QString value = formatter->format(elements);
            painter.save();
            QVariant maybeforeground = aggregator->categoryHeaderData(header.categoryIndex, Qt::ForegroundRole);
            if(maybeforeground.canConvert<QColor>()) {
                painter.setPen(maybeforeground.value<QColor>());
            } else if(maybebackground.canConvert<QPen>()) {
                painter.setPen(maybeforeground.value<QPen>());
            }
            painter.drawText(text_rect.adjusted(0,0,0,2), Qt::AlignCenter, value);
            painter.restore();
          text_rect.translate(0, text_rect.height());
        }
        summary_rect.translate(0, summary_rect.height());
      }
    }
    header_rect.translate(cell_size.width(), 0);
    summary_rect.translate(-cell_size.width(), 0);
  }
  if (vertical_header_count == 0) {
    header_rect.moveTop(q->viewport()->rect().top() + horizontal_header_height);
    header_rect.setSize(QSize(cell_size.width(), cell_size.height()));
    painter.drawRect(header_rect);
    if(show_totals) {
        summary_rect.moveTop(q->viewport()->rect().top() + horizontal_header_height);
        summary_rect.setSize(QSize(cell_size.width(), cell_size.height()));
        summary_rect.translate(cell_size.width()*2,0);
        painter.drawRect(summary_rect);
    }
  }

  // Draw grand total cell, if appropriate
  if (show_totals && bottommost_row >= ndatarows  && rightmost_column >= ndatacolumns ) {
      int adaptedVerticalHeaderCount = qMax(vertical_header_count,1);
      int adaptedHorizontalHeaderCount = qMax(horizontal_header_count,1);
    const int leftmost_summary = ndatacolumns-leftmost_column+adaptedVerticalHeaderCount;
    const int topmost_summary = ndatarows-topmost_row+adaptedHorizontalHeaderCount;
    summary_rect.moveTopLeft(q->viewport()->rect().topLeft() + QPoint(cell_size.width()*leftmost_summary, cell_size.height()*topmost_summary));
    summary_rect.setSize(QSize(cell_size.width() * adaptedVerticalHeaderCount, cell_size.height() * adaptedHorizontalHeaderCount));
    painter.drawRect(summary_rect);
    QRect text_rect(summary_rect);
    text_rect.translate(0, (summary_rect.height()-cell_size.height())/2); // Center vertically
    QList<int> elements = datacube->elements();
    Q_FOREACH(AbstractFormatter* formatter, formatters) {
      text_rect.setHeight(formatter->cellSize().height());
      const QString value = formatter->format(elements);
      painter.drawText(text_rect.adjusted(0,0,0,2), Qt::AlignCenter, value);
      text_rect.translate(0, text_rect.height());
    }
  }

  painter.setBrush(QBrush());
  painter.setPen(QPen());

  // Draw cells
  QBrush highlight = q->palette().highlight();
  QColor highligh_color = q->palette().color(QPalette::Highlight);
  QColor background_color = q->palette().color(QPalette::Window);
  QBrush faded_highlight = QColor((highligh_color.red() + background_color.red())/2,
                                   (highligh_color.green() + background_color.green())/2,
                                   (highligh_color.blue() + background_color.blue())/2);
  options.rect.moveTop(q->viewport()->rect().top() + horizontal_header_height);
  for (int r = q->verticalScrollBar()->value(), nr = qMin(datacube->rowCount(), bottommost_row+1); r < nr; ++r) {
    options.rect.moveLeft(q->viewport()->rect().left() + vertical_header_width);
    for (int c = q->horizontalScrollBar()->value(), nc = qMin(datacube->columnCount(), rightmost_column+1       ); c < nc; ++c) {
      DatacubeSelection::SelectionStatus selection_status = selection->selectionStatus(r, c);
      bool highlighted = false;
      switch (selection_status) {
        case DatacubeSelection::UNSELECTED:
          if (selection_area.contains(r, c)) {
            painter.fillRect(options.rect, highlight);
            highlighted = true;
          }
          break;
        case DatacubeSelection::SELECTED:
          if (selection_area.contains(r, c)) {
            painter.fillRect(options.rect, highlight);
            highlighted = true;
          } else {
            painter.fillRect(options.rect, highlight);
            highlighted = true;
          }
          break;
        case DatacubeSelection::PARTIALLY_SELECTED:
          if (selection_area.contains(r, c)) {
            painter.fillRect(options.rect, highlight);
            highlighted = true;
          } else {
            painter.fillRect(options.rect, faded_highlight);
          }
          break;
      }
      QList<int> elements = datacube->elements(r,c);
      if (elements.size() > 0) {
        QRect textrect(options.rect);
        Q_FOREACH(AbstractFormatter* formatter, formatters) {
          textrect.setHeight(formatter->cellSize().height());
          const QString value = formatter->format(elements);
          q->style()->drawItemText(&painter, textrect.adjusted(0,0,0,2), Qt::AlignCenter, q->palette(), true, value, highlighted ? QPalette::HighlightedText : QPalette::Text);
          textrect.translate(0,textrect.height());
        }
      }
      painter.drawRect(options.rect);
      options.rect.translate(cell_size.width(), 0);
    }
    options.rect.translate(0, cell_size.height());
  }

  event->setAccepted(true);
}

void DatacubeView::contextMenuEvent(QContextMenuEvent* event) {
  if(!d->datacube) {
    event->setAccepted(false);
    return;
  }
  QPoint pos = event->pos();
  if (pos.x() >= d->vertical_header_width + d->cell_size.width()*d->datacube->columnCount()) {
    event->setAccepted(false);
    return; // Right of datacube
  }
  if (pos.y() >= d->horizontal_header_height + d->cell_size.height()*d->datacube->rowCount()) {
    event->setAccepted(false);
    return;
  }
  event->accept();
  int column = (pos.x() - d->vertical_header_width) / d->cell_size.width() + horizontalScrollBar()->value();
  int row = (pos.y() - d->horizontal_header_height) / d->cell_size.height() + verticalScrollBar()->value();
  if (pos.y() < d->horizontal_header_height) {
    if (pos.x() >= d->vertical_header_width) {
      // Hit horizontal headers
      const int level = d->datacube->headerCount(Qt::Horizontal) - (d->horizontal_header_height - pos.y()) / d->cell_size.height() - 1;
      Q_ASSERT(level>=-1);
      int c = 0;
      int section = 0;
      if (level>=0) {
        Q_FOREACH(Datacube::HeaderDescription header, d->datacube->headers(Qt::Horizontal, level)) {
          if (c + header.span <= column) {
            ++section;
            c += header.span;
          } else {
            break;
          }
        }
      }
      c -= d->cell_size.width() * horizontalScrollBar()->value(); // Account for scrolled off headers
      QPoint header_element_pos = pos - QPoint(d->vertical_header_width + d->cell_size.width() * c, d->cell_size.height() * level);
      emit horizontalHeaderContextMenu(header_element_pos, level, section);
    } else {
      emit cornerContextMenu(pos);
    }
  } else {
    if (pos.x() < d->vertical_header_width) {
      int r = 0;
      int section = 0;
      int level = d->datacube->headerCount(Qt::Vertical) - (d->vertical_header_width - pos.x()) / d->cell_size.width() - 1;
      if (level>=0) {
        Q_FOREACH(Datacube::HeaderDescription header, d->datacube->headers(Qt::Vertical, level)) {
          if (r + header.span <= row) {
            ++section;
            r += header.span;
          } else {
            break;
          }
        }
      }
      r -= d->cell_size.height() * verticalScrollBar()->value(); // Account for scrolled off headers
      QPoint header_element_pos = pos - QPoint(d->cell_size.width() * level, d->horizontal_header_height + d->cell_size.height() * r);
      emit verticalHeaderContextMenu(header_element_pos, level, section);
    } else {
      // cells
      QPoint cell_pos = pos - QPoint(d->vertical_header_width + ( column - horizontalScrollBar()->value() ) * d->cell_size.width(),
                                     d->horizontal_header_height + ( row - verticalScrollBar()->value() ) * d->cell_size.height());
      cellContextMenu(cell_pos, row, column);
    }
  }

}

Datacube* DatacubeView::datacube() const {
  return d->datacube;
}

void DatacubeView::mousePressEvent(QMouseEvent* event) {
  QAbstractScrollArea::mousePressEvent(event);
  if (event->button() != Qt::LeftButton || !d->datacube) {
    event->setAccepted(false);
    return;
  }
  QPoint pos = event->pos();
  d->mouse_press_point = pos;
  d->mouse_press_scrollbar_state.setX(horizontalScrollBar()->value());
  d->mouse_press_scrollbar_state.setY(verticalScrollBar()->value());
  Cell press = d->cell_for_position(pos, verticalScrollBar()->value(), horizontalScrollBar()->value());
  if (!(event->modifiers() & Qt::CTRL)) {
    d->selection->clear();
  }
  if (event->modifiers() & Qt::SHIFT) {
    // In this case, simulate that the user actually first pressed on the lastly-clicked cell,
    // and then proceeded to drag the mouse to it's current position.
    // Doing it this way makes it very simple to implement the shift-press
    // handling.
    d->mouse_press_point = d->last_mouse_press_point;
    d->mouse_press_scrollbar_state = d->last_mouse_press_scrollbar_state;
    mouseMoveEvent(event);
    return;
  } else {
      // Save state for SHIFT-click behavior
    d->last_mouse_press_point = d->mouse_press_point;
    d->last_mouse_press_scrollbar_state = d->mouse_press_scrollbar_state;
  }

  const int vertical_header_count = d->datacube->headerCount(Qt::Vertical);
  const int horizontal_header_count = d->datacube->headerCount(Qt::Horizontal);

  if (press.invalid()) {
    d->selection_area = QRect();
  } else if (press.row()>=0 && press.row() < d->datacube_size.height()) {
    if (press.column()>=0 && press.column() < d->datacube_size.width()) {
      d->selection_area = QRect(press.row(), press.column(), 1, 1);
      d->header_selection_area = QRect();
    } else if ((press.column()<0 && -press.column() <= vertical_header_count) || press.column() >= d->datacube_size.width()) {
      const int headerno = press.column() < 0 ? vertical_header_count+press.column() : (press.column() - d->datacube_size.width());
      const int header_section = d->datacube->toHeaderSection(Qt::Vertical, headerno, press.row());
      QPair<int,int> row_range = d->datacube->toSection(Qt::Vertical, headerno, header_section);
      d->selection_area = QRect(row_range.first, 0, row_range.second - row_range.first + 1, d->datacube_size.width());
      d->header_selection_area = QRect(header_section, -headerno-1, 1,1);
    }
  } else if (press.column()>=0 && press.column() < d->datacube_size.width()) {
    if ((press.row()<0 && -press.row() <= horizontal_header_count) || press.row() >= d->datacube_size.height()) {
      const int headerno = press.row() < 0 ? horizontal_header_count+press.row() : (press.row() - d->datacube_size.height());
      const int header_section = d->datacube->toHeaderSection(Qt::Horizontal, headerno, press.column());
      QPair<int,int> column_range = d->datacube->toSection(Qt::Horizontal, headerno, header_section);
      d->selection_area = QRect(0, column_range.first, d->datacube_size.height(), column_range.second - column_range.first + 1);
      d->header_selection_area = QRect(-headerno-1, header_section, 1,1);

    }
  }
  viewport()->update();
  event->accept();
}

void DatacubeView::mouseMoveEvent(QMouseEvent* event) {
  QAbstractScrollArea::mouseMoveEvent(event);
  Cell press = d->cell_for_position(d->mouse_press_point, d->mouse_press_scrollbar_state.y(), d->mouse_press_scrollbar_state.x());
  QPoint constrained_pos = QPoint(qMax(1, qMin(event->pos().x(), d->vertical_header_width + d->datacube_size.width() * d->cell_size.width() + (d->show_totals ? d->vertical_header_width : 0) -1)),
                                  qMax(1, qMin(event->pos().y(), d->horizontal_header_height + d->datacube_size.height() * d->cell_size.height() + (d->show_totals ? d->horizontal_header_height : 0) - 1)));
  Cell current = d->cell_for_position(constrained_pos,verticalScrollBar()->value(), horizontalScrollBar()->value());
  QRect new_selection_area;
  if (!press.invalid() && !current.invalid()) {
    const int vertical_header_count = d->datacube->headerCount(Qt::Vertical);
    const int horizontal_header_count = d->datacube->headerCount(Qt::Horizontal);
    if (0<=press.row() && press.row() < d->datacube_size.height()) {
      if (0<=press.column() && press.column() < d->datacube_size.width()) {
        const int upper_row = qMin(press.row(), current.row());
        const int height = qAbs(press.row() - current.row()) + 1;
        const int left_column = qMin(press.column(), current.column());
        const int width = qAbs(press.column() - current.column()) + 1;
        new_selection_area = QRect(upper_row, left_column, height, width);
        d->header_selection_area = QRect();
      } else if (((vertical_header_count <= press.column() && press.column()) <= 0) || press.column() >= d->datacube_size.width()) {
        const int headerno = press.column() < 0 ? vertical_header_count+press.column() : (press.column() - d->datacube_size.width());
        int upper_header_section = d->datacube->toHeaderSection(Qt::Vertical, headerno, press.row());
        int lower_header_section = d->datacube->toHeaderSection(Qt::Vertical, headerno, qMax(0, qMin(current.row(), d->datacube_size.height()-1)));
        if (upper_header_section > lower_header_section) {
          std::swap(upper_header_section, lower_header_section);
        }
        const int upper_row = d->datacube->toSection(Qt::Vertical, headerno, upper_header_section).first;
        const int lower_row = d->datacube->toSection(Qt::Vertical, headerno, lower_header_section).second;
        const int left_column = 0;
        const int width = d->datacube_size.width();
        new_selection_area = QRect(upper_row, left_column, lower_row-upper_row+1, width);
        d->header_selection_area = QRect(upper_header_section, -headerno-1, lower_header_section-upper_header_section+1, 1);
      }
    } else if (press.column()>=0 && press.column() < d->datacube_size.width()) {
      if ((press.row()<0 && -press.row() <= horizontal_header_count) || press.row() >= d->datacube_size.height()) {
        const int headerno = press.row() < 0 ? horizontal_header_count+press.row() : (press.row() - d->datacube_size.height());
        int leftmost_header_section = d->datacube->toHeaderSection(Qt::Horizontal, headerno, press.column());
        int rightmost_header_section = d->datacube->toHeaderSection(Qt::Horizontal, headerno, qMax(0, qMin(current.column(), d->datacube_size.width()-1)));
        if (leftmost_header_section > rightmost_header_section) {
          std::swap(leftmost_header_section, rightmost_header_section);
        }
        const int leftmost_column = d->datacube->toSection(Qt::Horizontal, headerno, leftmost_header_section).first;
        const int rightmost_column = d->datacube->toSection(Qt::Horizontal, headerno, rightmost_header_section).second;
        const int upper_row = 0;
        const int height = d->datacube_size.height();
        new_selection_area = QRect(upper_row, leftmost_column, height, rightmost_column-leftmost_column+1);
        d->header_selection_area = QRect(-headerno-1, leftmost_header_section, 1, rightmost_header_section-leftmost_header_section+1);
      }
    }
  }
  if (new_selection_area != d->selection_area) {
    d->selection_area = new_selection_area;
    viewport()->update();
  }

}

void DatacubeView::mouseReleaseEvent(QMouseEvent* event) {
  QAbstractScrollArea::mouseReleaseEvent(event);
  if (!d->selection_area.isNull()) {
    QList<int> elementsToinclude;
    for (int r = d->selection_area.left(); r <= d->selection_area.right(); ++r) {
      for (int c = d->selection_area.top(); c <= d->selection_area.bottom(); ++c) {
        if (r >= 0 && c >= 0 && r < d->datacube_size.height() && c < d->datacube_size.width()) {
          QList<int> raw_elements = d->datacube->elements(r,c);
          elementsToinclude << raw_elements;
        } else {
          if (c >= d->datacube_size.width()) {
            //
          } else if (r>= d->datacube_size.height()) {
            //
          }

        }
      }
    }
    if(!elementsToinclude.isEmpty()) {
      d->selection->addElements(elementsToinclude);
    }
  } else {
    Cell release = d->cell_for_position(event->pos(), verticalScrollBar()->value(), horizontalScrollBar()->value());
    if (release.column()>=d->datacube_size.width() && release.row() >= d->datacube_size.height()) {
      Cell press = d->cell_for_position(d->last_mouse_press_point, d->last_mouse_press_scrollbar_state.y(), d->last_mouse_press_scrollbar_state.x());
      if (press.column()>=d->datacube_size.width() && press.row() >= d->datacube_size.height()) {
        // Lower right corner "total summary" was pressed. Select all
        d->selection->addElements(d->datacube->elements());
      }
    }
  }
  d->selection_area = QRect();
  d->header_selection_area = QRect();
  d->mouse_press_point = QPoint();
}

DatacubeSelection* DatacubeView::datacubeSelection() const
{
  return d->selection;
}

QRect DatacubeView::corner() const {
  return QRect(0,0,d->vertical_header_width, d->horizontal_header_height);
}

void DatacubeView::addFormatter(AbstractFormatter* formatter)
{
  d->formatters << formatter;
  connect(formatter,SIGNAL(cellSizeChanged(QSize)), d.data(), SLOT(relayout()));
  connect(formatter,SIGNAL(formatterChanged()), d.data(), SLOT(relayout()));
  formatter->setParent(this);
  d->relayout();
}

QList< AbstractFormatter* > DatacubeView::formatters() const
{
  return d->formatters;
}

AbstractFormatter* DatacubeView::takeFormatter(int index)
{
  AbstractFormatter* formatter = d->formatters.takeAt(index);
  disconnect(formatter,SIGNAL(cellSizeChanged(QSize)), d.data(),SLOT(relayout()));
  disconnect(formatter,SIGNAL(formatterChanged()), d.data(), SLOT(relayout()));
  d->relayout();
  return formatter;
}

void DatacubeViewPrivate::datacube_deleted() {
    selection->deleteLater();
    q->d.reset(new DatacubeViewPrivate(q));
}

bool DatacubeView::event(QEvent* event) {
    if(event->type() == QEvent::ToolTip) {
        do {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            Cell press = d->cell_for_position(helpEvent->pos(), verticalScrollBar()->value(), horizontalScrollBar()->value());
            if(press.invalid()) {
                // completely out of relevance
                break;
            }
            if(press.column() >= 0 && press.row() >= 0) {
                // within the cells
                break;
            }
            if(press.column() < 0 && press.row() < 0) {
                // top left corner
                QString toolTipText;
                Q_FOREACH(AbstractFilter::Ptr filter, d->datacube->filters()) {
                    if(!toolTipText.isEmpty()) {
                        toolTipText += QStringLiteral("\n");
                    }
                    toolTipText += filter->name();
                }
                if(!toolTipText.isEmpty()) {
                    QToolTip::showText(helpEvent->globalPos(), toolTipText);
                }
                break;
            }
            if(press.column() >= d->datacube->columnCount()) {
                // in the sums
                break;
            }
            if(press.row() >= d->datacube->rowCount()) {
                // in the sums
                break;
            }
            Qt::Orientation direction =  press.row() < 0 ? Qt::Horizontal : Qt::Vertical;

            int aggregatorNumber = (direction == Qt::Horizontal) ? (press.row() + d->datacube->headerCount(direction)) : (press.column() + d->datacube->headerCount(direction));

            AbstractAggregator::Ptr aggregator = (direction == Qt::Horizontal) ? d->datacube->columnAggregators().at(aggregatorNumber) : d->datacube->rowAggregators().at(aggregatorNumber);
            int category = d->datacube->categoryIndex(direction, aggregatorNumber, direction == Qt::Horizontal ? press.column() : press.row());

            QVariant toolTipVariant = aggregator->categoryHeaderData(category, Qt::ToolTipRole);

            if(toolTipVariant.isValid()) {
                QToolTip::showText(helpEvent->globalPos(), toolTipVariant.toString());
            }
        } while (0);
        event->accept();
        return true;
    }
    return QAbstractScrollArea::event(event);
}



} // end of namespace

#include "datacubeview.moc"
