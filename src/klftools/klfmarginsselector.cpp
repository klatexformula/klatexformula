/***************************************************************************
 *   file klfmarginsselector.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
 *   philippe.faist at bluewin.ch
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
/* $Id$ */

#include <algorithm> // std::min

#include "klfdefs.h"
#include "klfpopupsheet.h"

#include "klfmarginsselector.h"
#include "klfmarginsselector_p.h"



const double float_comparison_epsilon = 1e-6;


KLFMarginsEditor::KLFMarginsEditor(KLFPopupSheet * parent)
  : QWidget(parent)
{
  u = new Ui::KLFMarginsEditor;
  u->setupUi(this);

  connect(u->spnTop, SIGNAL(valueInRefUnitChanged(double)),
          this, SLOT(emit_updated_margins()));
  connect(u->spnRight, SIGNAL(valueInRefUnitChanged(double)),
          this, SLOT(emit_updated_margins()));
  connect(u->spnBottom, SIGNAL(valueInRefUnitChanged(double)),
          this, SLOT(emit_updated_margins()));
  connect(u->spnLeft, SIGNAL(valueInRefUnitChanged(double)),
          this, SLOT(emit_updated_margins()));

  connect(u->cbxUnits, SIGNAL(unitChanged(double, QString)),
          this, SLOT(show_unit_changed(double, QString)));

  connect(u->btnDone, SIGNAL(clicked()), this, SIGNAL(requestHide()));

  connect(parent, SIGNAL(popupShown()), this, SLOT(maybe_update_uniform_checked()));

  ignore_valuechange_event = false;
}

KLFMarginsEditor::~KLFMarginsEditor()
{
}


void KLFMarginsEditor::maybe_update_uniform_checked()
{
  qreal t = u->spnTop->valueInRefUnit();
  qreal r = u->spnRight->valueInRefUnit();
  qreal b = u->spnBottom->valueInRefUnit();
  qreal l = u->spnLeft->valueInRefUnit();
  qreal minval = std::min(std::min(std::min(t, r), b), l);
  qreal maxval = std::max(std::max(std::max(t, r), b), l);
  u->chkUniform->setChecked((maxval-minval) < float_comparison_epsilon);
}

void KLFMarginsEditor::emit_updated_margins()
{
  if (ignore_valuechange_event) {
    return;
  }

  if (u->chkUniform->isChecked()) {
    bool tmp = ignore_valuechange_event;
    ignore_valuechange_event = true;

    KLFUnitSpinBox * spnsender = dynamic_cast<KLFUnitSpinBox*>( sender() );
    qreal value = spnsender->valueInRefUnit();
    if (spnsender != u->spnTop) { u->spnTop->setValueInRefUnit(value); }
    if (spnsender != u->spnRight) { u->spnRight->setValueInRefUnit(value); }
    if (spnsender != u->spnBottom) { u->spnBottom->setValueInRefUnit(value); }
    if (spnsender != u->spnLeft) { u->spnLeft->setValueInRefUnit(value); }

    ignore_valuechange_event = tmp;
  }

  emit editorUpdatedMargins();
  //u->spnTop->valueInRefUnit(), u->spnRight->valueInRefUnit(),
  //u->spnBottom->valueInRefUnit(), u->spnLeft->valueInRefUnit());
}

void KLFMarginsEditor::show_unit_changed(double unitfactor, QString suffix)
{
  bool tmp = ignore_valuechange_event;
  ignore_valuechange_event = true;

  u->spnTop->setUnitWithSuffix(unitfactor, suffix);
  u->spnRight->setUnitWithSuffix(unitfactor, suffix);
  u->spnBottom->setUnitWithSuffix(unitfactor, suffix);
  u->spnLeft->setUnitWithSuffix(unitfactor, suffix);
  
  ignore_valuechange_event = tmp;
}






// main class

KLFMarginsSelector::KLFMarginsSelector(QWidget * parent)
  : QPushButton(parent)
{
  setCheckable(true);
  setText("Margins: <UNSET>");
  
  KLF_INIT_PRIVATE(KLFMarginsSelector) ;

  d->mPopupSheet = new KLFPopupSheet(this);
  d->mMarginsEditor = new KLFMarginsEditor(d->mPopupSheet);
  d->mPopupSheet->setCentralWidget(d->mMarginsEditor);

  d->mPopupSheet->associateWithButton(this);

  connect(d->mMarginsEditor, SIGNAL(requestHide()),
          d->mPopupSheet, SLOT(hidePopup()));
  connect(d->mMarginsEditor, SIGNAL(editorUpdatedMargins()),
          d, SLOT(updateMarginsDisplay()));

  d->updateMarginsDisplay();
}

KLFMarginsSelector::~KLFMarginsSelector()
{
  KLF_DELETE_PRIVATE ;
}

qreal KLFMarginsSelector::topMargin()
{
  return d->mMarginsEditor->u->spnTop->valueInRefUnit();
}
qreal KLFMarginsSelector::rightMargin()
{
  return d->mMarginsEditor->u->spnRight->valueInRefUnit();
}
qreal KLFMarginsSelector::bottomMargin()
{
  return d->mMarginsEditor->u->spnBottom->valueInRefUnit();
}
qreal KLFMarginsSelector::leftMargin()
{
  return d->mMarginsEditor->u->spnLeft->valueInRefUnit();
}

void KLFMarginsSelector::setMargins(qreal t, qreal r, qreal b, qreal l)
{
  qreal minval = std::min(std::min(std::min(t, r), b), l);
  qreal maxval = std::max(std::max(std::max(t, r), b), l);

  {
    QSignalBlocker blocker(d->mMarginsEditor);
    d->mMarginsEditor->u->spnTop->setValueInRefUnit(t);
    d->mMarginsEditor->u->spnRight->setValueInRefUnit(r);
    d->mMarginsEditor->u->spnBottom->setValueInRefUnit(b);
    d->mMarginsEditor->u->spnLeft->setValueInRefUnit(l);
    d->mMarginsEditor->u->chkUniform->setChecked( (maxval - minval) < float_comparison_epsilon );
  }

  d->updateMarginsDisplay();
}


// void KLFMarginsSelectorPrivate::editorButtonToggled(bool toggled)
// {
//   if (toggled) {
//     mMarginsEditor->setVisible(true);
//   } else {
//     mMarginsEditor->setVisible(false);
//   }
// }

// void KLFMarginsSelectorPrivate::editorVisibilityChanged(bool on)
// {
//   K->setChecked(on);
// }

void KLFMarginsSelectorPrivate::updateMarginsDisplay()
{
  qreal t = K->topMargin();
  qreal r = K->rightMargin();
  qreal b = K->bottomMargin();
  qreal l = K->leftMargin();

  qreal minval = std::min(std::min(std::min(t, r), b), l);
  qreal maxval = std::max(std::max(std::max(t, r), b), l);

  if ((maxval - minval) < float_comparison_epsilon) {
    K->setText(QString::fromLatin1(klfFmt("Margins: %.2g pt", (double)minval)));
  } else {
    K->setText(QString::fromLatin1(klfFmt("top %.2gpt/right %.2gpt/bottom %.2gpt/left %.2gpt",
                                          (double)t, (double)r, (double)b, (double)l)));
  }
}
