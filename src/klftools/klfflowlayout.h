
#ifndef KLFFLOWLAYOUT_H
#define KLFFLOWLAYOUT_H

#include <QLayout>


class KLFFlowLayoutPrivate;

class KLFFlowLayout : public QLayout
{
  Q_OBJECT

  Q_ENUMS(Flush)
  Q_PROPERTY(Flush flush READ flush WRITE setFlush) ;
  Q_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing) ;
  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing) ;

public:
  /** How to deal with too much space: */
  enum Flush {
    NoFlush = 0, //!< Give the extra space to the widgets, don't flush.
    FlushSparse, //!< Distribute the extra space between widgets to fill the line
    FlushBegin, //!< Leave all extra space at end of line
    FlushEnd //!< Leave all extra space at beginning of line
  };

  KLFFlowLayout(QWidget *parent, int margin = -1, int hspacing = -1, int vspacing = -1);
  virtual ~KLFFlowLayout();

  /** Add a QLayoutItem to the layout. Ownership of the object is taken by the layout: it will be deleted
   * by the layout. */
  virtual void addItem(QLayoutItem *item)
  { addItem(item, 0, 0); }
  virtual void addItem(QLayoutItem *item, int hstretch, int vstretch);
  virtual void addLayout(QLayout *l, int hstretch = 0, int vstretch = 0);
  virtual void addWidget(QWidget *w, int hstretch = 0, int vstretch = 0, Qt::Alignment align = 0);
  int horizontalSpacing() const;
  void setHorizontalSpacing(int spacing);
  int verticalSpacing() const;
  void setVerticalSpacing(int spacing);
  Flush flush() const;
  void setFlush(Flush f);
  virtual int count() const;
  virtual QLayoutItem *itemAt(int index) const;
  virtual QLayoutItem *takeAt(int index);
  virtual Qt::Orientations expandingDirections() const;
  virtual bool hasHeightForWidth() const;
  virtual int heightForWidth(int width) const;
  virtual QSize minimumSize() const;
  virtual QSize maximumSize() const;
  virtual QSize sizeHint() const;

  void setGeometry(const QRect &rect);

  virtual void invalidate();

  virtual bool event(QEvent *event);
  virtual bool eventFilter(QObject *obj, QEvent *event);
  
private:

  KLFFlowLayoutPrivate *d;
};







#endif
