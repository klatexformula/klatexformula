/***************************************************************************
 *   file klfstylemanager.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>

#include <qapplication.h>
#include <qmemarray.h>
#include <qdatastream.h>
#include <qvaluelist.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kstandarddirs.h>

#include "klfstylemanager.h"


KLFStyleDrag::KLFStyleDrag(KLFData::KLFStyle sty, QWidget *parent)
  : QDragObject(parent)
{
  _sty = sty;
}

const char *KLFStyleDrag::format(int index) const
{
  switch (index) {
  case 0:
    return "application/x-klatexformula-style";
  default:
    return 0;
  }
}

QByteArray KLFStyleDrag::encodedData(const char *format) const
{
  QByteArray data;
  if (!strcmp(format, "application/x-klatexformula-style")) {
    QDataStream str(data, IO_WriteOnly);
    str << _sty;
  }
  return data;
}

bool KLFStyleDrag::canDecode(const QMimeSource *source)
{
  return source->provides("application/x-klatexformula-style");
}

bool KLFStyleDrag::decode(const QMimeSource *src, KLFData::KLFStyle& sty)
{
  QByteArray data = src->encodedData("application/x-klatexformula-style");
  QDataStream str(data, IO_ReadOnly);
  str >> sty;
  return true;
}





// ------------------------------------



KLFStyleManager::KLFStyleManager(KLFData::KLFStyleList *stydata, QWidget *parent)
  : KLFStyleManagerUI(parent, 0, false, 0)
{
  lstStyles->installEventFilter(this);

  _styptr = stydata;

  _drag_item = 0;
  _drag_init_pos = QPoint(-1,-1);
  /*  mDropIndicatorItem = 0; */

  mActionsPopup = new KPopupMenu(this);

  mActionsPopup->clear();

  mActionsPopup->insertTitle(i18n("Style Management Actions"), 100000);
  mActionsPopup->insertItem(i18n("Delete Style"), this, SLOT(slotDelete()), 0, PopupIdDelete);
  mActionsPopup->insertItem(i18n("Move up"), this, SLOT(slotMoveUp()), 0, PopupIdMoveUp);
  mActionsPopup->insertItem(i18n("Move down"), this, SLOT(slotMoveDown()), 0, PopupIdMoveDown);
  mActionsPopup->insertItem(i18n("Rename style"), this, SLOT(slotRename()), 0, PopupIdRename);

  btnActions->setPopup(mActionsPopup);

  // populate style list
  stylesChanged();

  // and set menu items enabled or not
  refreshActionsEnabledState();

  connect(btnClose, SIGNAL(clicked()), this, SLOT(hide()));

  connect(lstStyles, SIGNAL(highlighted(int)), this, SLOT(refreshActionsEnabledState()));
  connect(lstStyles, SIGNAL(selected(int)), this, SLOT(refreshActionsEnabledState()));
  connect(lstStyles, SIGNAL(currentChanged(QListBoxItem *)), this, SLOT(refreshActionsEnabledState()));
}

KLFStyleManager::~KLFStyleManager()
{
}

void KLFStyleManager::refreshActionsEnabledState()
{
  if (lstStyles->selectedItem() != 0) {
    lstStyles->setCurrentItem(lstStyles->index(lstStyles->selectedItem()));
  }
  if (lstStyles->currentItem() != -1) {
    mActionsPopup->setItemEnabled(PopupIdDelete, true);
    mActionsPopup->setItemEnabled(PopupIdRename, true);
    mActionsPopup->setItemEnabled(PopupIdMoveUp, lstStyles->currentItem() > 0);
    mActionsPopup->setItemEnabled(PopupIdMoveDown, lstStyles->currentItem() < ((int)lstStyles->count()-1));
  } else {
    mActionsPopup->setItemEnabled(PopupIdDelete, false);
    mActionsPopup->setItemEnabled(PopupIdRename, false);
    mActionsPopup->setItemEnabled(PopupIdMoveUp, false);
    mActionsPopup->setItemEnabled(PopupIdMoveDown, false);
  }
}

void KLFStyleManager::slotDelete()
{
  int ind = lstStyles->currentItem();
  if ( ind >= 0 &&
       KMessageBox::questionYesNo(this, i18n("Are you sure you want to erase selected style?"),
				  i18n("Erase style?")) == KMessageBox::Yes ) {
    delete lstStyles->item(ind);
    _styptr->erase(_styptr->at(ind));
  }

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotRename()
{
  int ind = lstStyles->currentItem();
  if (ind < 0)
    return;

  QString newname = KInputDialog::getText(i18n("Enter new style name:"), i18n("Rename style"), _styptr->operator[](ind).name, 0, this);

  if (!newname.isEmpty()) {
    _styptr->operator[](ind).name = newname;
    delete lstStyles->item(ind);
    lstStyles->insertItem(newname, ind);
  }

  lstStyles->setSelected(ind, true);
  lstStyles->setCurrentItem(ind);

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveUp()
{
  int ind = lstStyles->currentItem();
  if (ind < 1)
    return;

  KLFData::KLFStyle s = _styptr->operator[](ind);
  _styptr->operator[](ind) = _styptr->operator[](ind-1);
  _styptr->operator[](ind-1) = s;

  QListBoxItem *it;
  lstStyles->takeItem(it = lstStyles->item(ind));
  lstStyles->insertItem(it, ind-1);
  lstStyles->setSelected(ind, false);
  lstStyles->setSelected(ind-1, true);
  lstStyles->setCurrentItem(ind-1);

  emit refreshStyles();
  refreshActionsEnabledState();
}

void KLFStyleManager::slotMoveDown()
{
  int ind = lstStyles->currentItem();
  if (ind < 0 || ind >= ((int)lstStyles->count()-1))
    return;

  KLFData::KLFStyle s = _styptr->operator[](ind);
  _styptr->operator[](ind) = _styptr->operator[](ind+1);
  _styptr->operator[](ind+1) = s;

  QListBoxItem *it;
  lstStyles->takeItem(it = lstStyles->item(ind));
  lstStyles->insertItem(it, ind+1);
  lstStyles->setSelected(ind, false);
  lstStyles->setSelected(ind+1, true);
  lstStyles->setCurrentItem(ind+1);

  emit refreshStyles();
  refreshActionsEnabledState();
}


bool KLFStyleManager::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == lstStyles) { // style list received an event
    if (event->type() == QEvent::Enter) {
      refreshActionsEnabledState();
      // don't return, let our parent process stuff
    }
    else if (event->type() == QEvent::DragEnter) {
      QDragEnterEvent *drevent = (QDragEnterEvent *) event;
      if (KLFStyleDrag::canDecode(drevent)) {
	drevent->accept(true);
	return true;
      }
      /* mDropIndicatorItem = new QListBoxPixmap(lstStyles, QPixmap(locate("appdata", "pics/hbar.png")));
	 mDropIndicatorItem->setSelectable(false); */
    }
    else if (event->type() == QEvent::DragMove) {
      /*      if (mDropIndicatorItem == 0) { // create our indicator
	      mDropIndicatorItem = new QListBoxPixmap(lstStyles, QPixmap(locate("appdata", "pics/hbar.png")));
	      mDropIndicatorItem->setSelectable(false);
	      }
	      QDragMoveEvent *drmev = (QDragMoveEvent *) event;
	      QListBoxItem *it = lstStyles->itemAt(drmev->pos());
	      int i = lstStyles->index(it);
	      if (mDropIndicatorItem && lstStyles->index(mDropIndicatorItem) != -1)
	      lstStyles->takeItem(mDropIndicatorItem);
	      if (mDropIndicatorItem)
	      lstStyles->insertItem(mDropIndicatorItem, i);
	      lstStyles->triggerUpdate(true);
	      return true;
      */
    }
    else if (event->type() == QEvent::Drop) {
      QDropEvent *dropevent = (QDropEvent *) event;
      KLFData::KLFStyle sty;
      if (KLFStyleDrag::decode(dropevent, sty)) {
	QListBoxItem *it = lstStyles->itemAt(dropevent->pos());
	// 'it' should be 'mDropIndicatorItem'

	int i = lstStyles->index(it);
	// if i==-1, keep it at i==-1 so insert goes to end
	if (i >= 0) {
	  _styptr->insert(_styptr->at(i), sty);
	  lstStyles->insertItem(sty.name, i);
	} else {
	  _styptr->append(sty);
	  lstStyles->insertItem(sty.name, -1);
	}
	emit refreshStyles();
      }
      /*      if (mDropIndicatorItem) delete mDropIndicatorItem;
	      mDropIndicatorItem = 0; */
      return true;
    }
    else if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = (QMouseEvent *) event;
      if (me->button() == LeftButton) {
	_drag_init_pos = me->pos();
	_drag_item = lstStyles->itemAt(_drag_init_pos);
	if (lstStyles->itemAt(_drag_init_pos) == 0) {
	  _drag_init_pos = QPoint(-1,-1);
	  _drag_item = 0;
	}
	lstStyles->setCurrentItem(_drag_item);
	return true;
      }
    }
    else if (event->type() == QEvent::MouseMove && _drag_init_pos.x() != -1 && _drag_item != 0) {
      QMouseEvent *me = (QMouseEvent *) event;
      if ( (me->state() & LeftButton) == LeftButton &&
	   (me->pos() - _drag_init_pos).manhattanLength() > QApplication::startDragDistance() &&
	   lstStyles->currentItem() != -1) {
	KLFStyleDrag *dr = new KLFStyleDrag(_styptr->operator[](lstStyles->currentItem()), this);
	dr->drag();
	if (dr->target() == lstStyles || dr->target() == this) {
	  int ind = lstStyles->index(_drag_item);
	  if (ind >= 0) {
	    lstStyles->removeItem(ind);
	    _styptr->erase(_styptr->at(ind));
	    emit refreshStyles();
	  }
	}
	me->accept();
	return true;
      }
    }
  }
  return KLFStyleManagerUI::eventFilter(obj, event);
}


void KLFStyleManager::stylesChanged()
{
  lstStyles->clear();
  for (uint i = 0; i < _styptr->size(); ++i) {
    lstStyles->insertItem(_styptr->operator[](i).name);
  }
}



#include "klfstylemanager.moc"

