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

#include "klfmarginsselector.h"
#include "klfmarginsselector_p.h"



#define FLOAT_COMPARISON_EPSILON 1e-6


KLFMarginsEditor::KLFMarginsEditor(QWidget * parent, bool use_dialog_)
  : QWidget(parent, use_dialog_ ? Qt::Dialog : Qt::Popup),
    use_dialog(use_dialog_)
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

  connect(u->btnDone, SIGNAL(clicked()), this, SLOT(hide()));

  ignore_valuechange_event = false;
}

KLFMarginsEditor::~KLFMarginsEditor()
{
}

void KLFMarginsEditor::setVisible(bool on)
{
  if (on) {
    move(parentWidget()->mapToGlobal(parentWidget()->geometry().bottomLeft()));
  }
  QWidget::setVisible(on);


  qreal t = u->spnTop->valueInRefUnit();
  qreal r = u->spnRight->valueInRefUnit();
  qreal b = u->spnBottom->valueInRefUnit();
  qreal l = u->spnLeft->valueInRefUnit();
  qreal minval = std::min(std::min(std::min(t, r), b), l);
  qreal maxval = std::max(std::max(std::max(t, r), b), l);
  u->chkUniform->setChecked((maxval-minval) < FLOAT_COMPARISON_EPSILON);

  emit visibilityChanged(on);
}

void KLFMarginsEditor::emit_updated_margins()
{
  if (ignore_valuechange_event) {
    return;
  }

  if (u->chkUniform->isChecked()) {
    ignore_valuechange_event = true;

    KLFUnitSpinBox * spnsender = dynamic_cast<KLFUnitSpinBox*>( sender() );
    qreal value = spnsender->valueInRefUnit();
    if (spnsender != u->spnTop) { u->spnTop->setValue(value); }
    if (spnsender != u->spnRight) { u->spnRight->setValue(value); }
    if (spnsender != u->spnBottom) { u->spnBottom->setValue(value); }
    if (spnsender != u->spnLeft) { u->spnLeft->setValue(value); }

    ignore_valuechange_event = false;
  }

  emit editorUpdatedMargins();
  //u->spnTop->valueInRefUnit(), u->spnRight->valueInRefUnit(),
  //u->spnBottom->valueInRefUnit(), u->spnLeft->valueInRefUnit());
}

void KLFMarginsEditor::show_unit_changed(double unitfactor, QString suffix)
{
  ignore_valuechange_event = true;

  u->spnTop->setUnitWithSuffix(unitfactor, suffix);
  u->spnRight->setUnitWithSuffix(unitfactor, suffix);
  u->spnBottom->setUnitWithSuffix(unitfactor, suffix);
  u->spnLeft->setUnitWithSuffix(unitfactor, suffix);
  
  ignore_valuechange_event = false;
}






// main class

KLFMarginsSelector::KLFMarginsSelector(QWidget * parent)
  : QPushButton(parent)
{
  setCheckable(true);
  setText("Margins: <UNSET>");
  
  KLF_INIT_PRIVATE(KLFMarginsSelector) ;

  d->mMarginsEditor = new KLFMarginsEditor(this);

  connect(this, SIGNAL(toggled(bool)), d, SLOT(editorButtonToggled(bool)));
  connect(d->mMarginsEditor, SIGNAL(visibilityChanged(bool)), d, SLOT(editorVisibilityChanged(bool)));

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

  d->mMarginsEditor->blockSignals(true);
  d->mMarginsEditor->u->spnTop->setValueInRefUnit(t);
  d->mMarginsEditor->u->spnRight->setValueInRefUnit(r);
  d->mMarginsEditor->u->spnBottom->setValueInRefUnit(b);
  d->mMarginsEditor->u->spnLeft->setValueInRefUnit(l);
  d->mMarginsEditor->u->chkUniform->setChecked( (maxval - minval) < FLOAT_COMPARISON_EPSILON );
  d->mMarginsEditor->blockSignals(false);
  d->updateMarginsDisplay();
}


void KLFMarginsSelectorPrivate::editorButtonToggled(bool toggled)
{
  if (toggled) {
    mMarginsEditor->setVisible(true);
  } else {
    mMarginsEditor->setVisible(false);
  }
}

void KLFMarginsSelectorPrivate::editorVisibilityChanged(bool on)
{
  K->setChecked(on);
}

void KLFMarginsSelectorPrivate::updateMarginsDisplay()
{
  qreal t = K->topMargin();
  qreal r = K->rightMargin();
  qreal b = K->bottomMargin();
  qreal l = K->leftMargin();

  qreal minval = std::min(std::min(std::min(t, r), b), l);
  qreal maxval = std::max(std::max(std::max(t, r), b), l);

  if ((maxval - minval) < FLOAT_COMPARISON_EPSILON) {
    K->setText(QString::fromLatin1(klfFmt("Margins: %.2g pt", (double)minval)));
  } else {
    K->setText(QString::fromLatin1(klfFmt("top %.2gpt/right %.2gpt/bottom %.2gpt/left %.2gpt",
                                          (double)t, (double)r, (double)b, (double)l)));
  }
}
