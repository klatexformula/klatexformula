/***************************************************************************
 *   file klfsidewidget.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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

#ifndef KLFSIDEWIDGET_H
#define KLFSIDEWIDGET_H


#include <QObject>
#include <QWidget>
#include <QPointer>

#include <klfdefs.h>
#include <klffactory.h>


struct KLFSideWidgetManagerBasePrivate;

//! Abstract core functionality of showing/hiding a KLFSideWidget
/** This class provides the interface for writing show/hide side widget managers.
 * Side widget managers are responsible for managing the whole show/hide process of the side widget; this includes
 * creating an extra container widget if nessecary (eg. a drawer on mac), initializing it, reparenting the side
 * widget if needed, etc.
 *
 * Three standard side widget managers are provided. KLFShowHideSideWidgetManager (like KLatexFormula
 * expanded mode on windows/linux) simply shows or hides the side widget inside its original layout inside
 * its parent. KLFFloatSideWidgetManager creates a floating tool window which contains the side widget.
 * On Mac OS X, there is also KLFDrawerSideWidgetManager which creates a Mac OS X drawer associated to the
 * original main widget.
 *
 * In general, you will not need to interact with this class directly. Everything is done transparently through
 * \ref KLFSideWidget::setSideWidgetManager().
 *
 * However, you may subclass this class to implement your own, customized, side widget managers. Consider
 * subclassing KLFContainerSideWidgetManager or one of the above mentioned managers to profit from their
 * functionality if you need it. Note the following points.
 *
 * - originally, the parentWidget given to the constructor is the parent of the side widget. However, you are
 *   free to reparent the widget to use it at your needs, but you will have to restore the widget to its original
 *   state when the manager is destroyed, the widget removed or replaced by another widget. See code for
 *   \ref KLFContainerSideWidgetManager for an example. If you reparent the side widget, make sure you pass
 *   FALSE to the 'requireSideWidgetParentConsistency' parameter of this class' constructor.
 * - sideWidgetVisible() needs not reflect sideWidget()->isVisible(); indeed, if you reparented your side widget,
 *   it might be the container that you're showing and hiding.
 */
class KLF_EXPORT KLFSideWidgetManagerBase : public QObject
{
  Q_OBJECT
public:
  /**
   * \warning This function does NOT initialize its internal pParentWidget and pSideWidget members---they
   *   are always initialized to NULL. Reason: subclasses should set these in their constructors via
   *   setSideWidget() and setOurParentWidget(), which then call newSideWidgetSet() etc., which passes
   *   through the virtual call (it does not in the base class constructor). */
  KLFSideWidgetManagerBase(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL,
			   bool requireSideWidgetParentConsistency = false, QObject *managerParent = NULL);
  virtual ~KLFSideWidgetManagerBase();

  /** returns the side widget set by \ref setSideWidget(). */
  virtual QWidget * sideWidget() const;
  /** returns the side widget set by \ref setOurParentWidget(). */
  virtual QWidget * ourParentWidget() const;

  /** Sets the side widget we will be managing to \c widget. If parent side widget consistency is set
   * to TRUE (see constructor), then the parent of the new widget is adjusted to ourParentWidget() if
   * necessary.
   */
  void setSideWidget(QWidget *widget);
  /** Changes our main widget to \c widget. If we're in parent side widget consistency mode, also adjusts
   * the parent of the current side widget. */
  void setOurParentWidget(QWidget *widget);

  //! returns TRUE if the side widget is currently visible
  /** Subclasses must reimplement to provide functionality. The base class has no way of knowing if the
   * side widget is considered shown or hidden.
   */
  virtual bool sideWidgetVisible() const = 0;

signals:
  /** \brief Emitted whenever the shown status of the side widget has changed.
   *
   * Subclasses must emit this signal in their showSideWidget() implementation. It is also up to the
   * subclass to emit it in their newSideWidgetSet() implementation with a consistent initial value.  */
  void sideWidgetShown(bool shown);

public slots:
  //! Show or hide the side widget
  /** Subclasses must implement here their method of showing and hiding the side widget. This might
   * be, for example, showing or hiding an external container widget which contains the side widget.
   *
   * Note that once you've reimplemented showSideWidget(bool), then showSideWidget(), hideSideWidget(bool),
   * and toggleSideWidget() automatically work correctly by calling your reimplementation.
   */
  virtual void showSideWidget(bool show) = 0;

  /** a shortcut for <code>showSideWidget(true)</code>. Needs not be reimplemented by subclasses. */
  void showSideWidget() { showSideWidget(true); }
  /** a shortcut for <code>showSideWidget(!hide)</code>. Needs not be reimplemented by subclasses. */
  void hideSideWidget(bool hide = true) { showSideWidget(!hide); }
  /** a shortcut for <code>showSideWidget(!sideWidgetVisible())</code>. Needs not be reimplemented by
   * subclasses. */
  void toggleSideWidget() { showSideWidget(!sideWidgetVisible()); }

  /** In case there is an animation for showing/hiding the widget, you may call this function to wait
   * until the animation is done. See also \ref showHideIsAnimating().
   *
   * This function needs not be reimplemented. The default implementation loops (while not freezing the
   * GUI) and periodically checks for showHideIsAnimating(), until that function returns FALSE or the
   * given timeout is reached.
   */
  virtual void waitForShowHideActionFinished(int timeout_ms = 2000);

protected:
  /** Called by setSideWidget(), once the new widget is set. */
  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget)
  {  Q_UNUSED(oldSideWidget); Q_UNUSED(newSideWidget);  }
  /** Called by setOurParentWidget(), once the new widget is set. */
  virtual void newParentWidgetSet(QWidget *oldParentWidget, QWidget *newParentWidget)
  {  Q_UNUSED(oldParentWidget); Q_UNUSED(newParentWidget); }

  /** In case you are implementing an animation for the show/hide process, then this function should
   * return TRUE as long as the animation is in progress, and should return FALSE once the animation is
   * done.
   *
   * See also \ref waitForShowHideActionFinished().
   */
  virtual bool showHideIsAnimating() { return false; }

private:
  KLF_DECLARE_PRIVATE(KLFSideWidgetManagerBase);
};


// -----


struct KLFShowHideSideWidgetManagerPrivate;

//! A show-widget/hide-widget side widget show/hide manager
/** \warning NOTE: You do not have to instantiate this class yourself to use KLFSideWidget. This is automatically
 * done in \ref KLFSideWidget::setSideWidgetManager().
 *
 * This class is provided in case you want to subclass it and use part of its functionality to write custom, more
 * advanced side widget managers. See also \ref KLFSideWidgetManagerBase.
 *
 * This manager just shows and hides the widget inside its original parent layout, and adapts the parent's size
 * correctly upon each show/hide.
 */
class KLF_EXPORT KLFShowHideSideWidgetManager : public KLFSideWidgetManagerBase
{
  Q_OBJECT

  Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation) ;
  Q_PROPERTY(int calcSpacing READ calcSpacing WRITE setCalcSpacing) ;
public:
  KLFShowHideSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL,
			       QObject *managerParent = NULL);
  virtual ~KLFShowHideSideWidgetManager();

  Qt::Orientation orientation() const;
  int calcSpacing() const;

  virtual bool eventFilter(QObject *obj, QEvent *event);

  virtual bool sideWidgetVisible() const;

public slots:
  virtual void showSideWidget(bool show);
  void setOrientation(Qt::Orientation o);
  void setCalcSpacing(int cs);

protected:
  virtual bool event(QEvent *event);

  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget);
  virtual void newParentWidgetSet(QWidget *oldParentWidget, QWidget *newParentWidget);

private slots:
  void resizeParentWidget(const QSize& size);

private:
  KLF_DECLARE_PRIVATE(KLFShowHideSideWidgetManager);
};


// ------

struct KLFContainerSideWidgetManagerPrivate;

//! A generic container side widget show/hide manager (mac only)
/** \warning NOTE: You do not have to instantiate this class yourself to use KLFSideWidget. This is automatically
 * done in \ref KLFSideWidget::setSideWidgetManager().
 *
 * This class is provided in case you want to subclass it and use part of its functionality to write custom, more
 * advanced side widget managers. See also \ref KLFSideWidgetManagerBase.
 *
 * This class provides a generic container-based side widget manager, meant to be subclassed for more
 * specific functionality. For example, both KLFFloatSideWidgetManager and KLFDrawerSideWidgetManager inherit
 * this class.
 *
 * Subclasses must reimplement createContainerWidget() to actually create the widget which will contain the
 * side widget. Also, subclasses must not forget to call init() in their constructor.
 */
class KLFContainerSideWidgetManager : public KLFSideWidgetManagerBase
{
  Q_OBJECT

public:
  KLFContainerSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL,
				QObject *managerParent = NULL);
  virtual ~KLFContainerSideWidgetManager();

  virtual bool sideWidgetVisible() const;

  bool eventFilter(QObject *obj, QEvent *event);

public slots:
  virtual void showSideWidget(bool show);

protected:
  /** \brief Must be called in subclasses' constructor */
  void init();

  /** \brief Must be reimplemented to create the container widget */
  virtual QWidget * createContainerWidget(QWidget * pw) = 0;

  virtual QWidget * containerWidget() const;

  virtual void newSideWidgetSet(QWidget *oldSideWidget, QWidget *newSideWidget);
  virtual void newParentWidgetSet(QWidget *oldWidget, QWidget *newWidget);

private slots:
  void aWidgetDestroyed(QObject *);

private:
  KLF_DECLARE_PRIVATE(KLFContainerSideWidgetManager) ;
};


// ------

// -------

struct KLFFloatSideWidgetManagerPrivate;

//! A Floating window show/hide manager.
/** \warning NOTE: You do not have to instantiate this class yourself to use KLFSideWidget. This is automatically
 * done in \ref KLFSideWidget::setSideWidgetManager().
 *
 * This class is provided in case you want to subclass it and use part of its functionality to write custom, more
 * advanced side widget managers. See also \ref KLFSideWidgetManagerBase.
 */
class KLF_EXPORT KLFFloatSideWidgetManager : public KLFContainerSideWidgetManager
{
  Q_OBJECT

  Q_PROPERTY(Qt::WindowFlags wflags READ wflags WRITE setWFlags) ;
public:
  KLFFloatSideWidgetManager(QWidget *parentWidget = NULL, QWidget *sideWidget = NULL,
			    QObject *managerParent = NULL);
  virtual ~KLFFloatSideWidgetManager();

  Qt::WindowFlags wflags() const;

  virtual bool sideWidgetVisible() const;

public slots:
  virtual void showSideWidget(bool show);
  void setWFlags(Qt::WindowFlags wflags);

protected:
  void newSideWidgetSet(QWidget *oldw, QWidget *w);

  virtual QWidget * createContainerWidget(QWidget * pw);

private:
  KLF_DECLARE_PRIVATE(KLFFloatSideWidgetManager) ;
};



// ------


/** \brief A factory for creating side widget managers.
 *
 *
 * \note This class acts as base class for any factory of side-widget-managers, as well as
 *   a functional instantiable factory for the built-in types.
 *
 * \note When reimplementing this class, it is automatically registered upon instanciation by
 *   the KLFFactoryBase base class.
 */
class KLF_EXPORT KLFSideWidgetManagerFactory : public KLFFactoryBase
{
public:
  KLFSideWidgetManagerFactory();
  virtual ~KLFSideWidgetManagerFactory();

  virtual QStringList supportedTypes() const;
  /** \brief A human-readable title to display as label of given type, e.g. in combo box */
  virtual QString getTitleFor(const QString& type) const;
  virtual KLFSideWidgetManagerBase * createSideWidgetManager(const QString& type, QWidget *parentWidget,
							     QWidget *sideWidget, QObject *parent);

  static QStringList allSupportedTypes();
  static KLFSideWidgetManagerBase * findCreateSideWidgetManager(const QString& type, QWidget *parentWidget,
								QWidget *sideWidget, QObject *parent);
  static KLFSideWidgetManagerFactory * findFactoryFor(const QString& managertype);

private:
  static KLFFactoryManager pFactoryManager;
};




// -----------------------------



struct KLFSideWidgetPrivate;

//! A widget that can be shown or hidden, that expands a main widget, e.g. klatexformula expanded mode
/** This container may be used to provide a widget that can be shown or hidden. This container makes
 * transparent the way this widget will be shown or hidden; for example it could be implemented as a Mac OS X
 * drawer, as a floating tool window, or shown alongside the main widget.
 *
 * A side widget manager (subclass of \ref KLFSideWidgetManagerBase) is responsible for actually implementing
 * the show/hide process. This includes instantiating a new floating widget/making it a drawer/reparenting
 * that and this widget etc. if needed. You may also implement your own
 * KLFSideWidgetManagerBase, and write a factory for it. See \ref KLFSideWidgetManagerBase. 
 * 
 * A KLFSideWidget has to be associated with a "Main Widget", which is our given by our parent widget at
 * the moment a side widget manager is set. In fact, it should be instantiated as a child of your "main widget",
 * possibly in a layout (especially for show/hide manager).
 *
 * A KLFSideWidget may also be created in Qt designer, inside the main widget UI.
 *
 * Minimal example:
 * \code
 *   QWidget *mainWidget = ...;
 *   Q[Something]Layout *mainLayout = ...;
 *   KLFSideWidget * sideWidget = new KLFSideWidget(mainWidget);
 *   mainLayout->addWidget(sideWidget);
 *
 *   sideWidget->setSideWidgetManager(KLFSideWidget::Float);
 *   // or
 *   sideWidget->setSideWidgetManager("Float");
 *
 *   // use eg. sideWidget->showSideWidget(bool)  to show/hide the side widget.
 * \endcode
 */
class KLF_EXPORT KLFSideWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(QString sideWidgetManagerType READ sideWidgetManagerType WRITE setSideWidgetManager) ;
public:
  enum SideWidgetManager { ShowHide = 1, Float, Drawer } ;

  //  KLFSideWidget(SideWidgetManager mtype = Float, QWidget *parent = NULL);
  //  KLFSideWidget(const QString& mtype = QLatin1String("Float"), QWidget *parent = NULL);
  KLFSideWidget(QWidget *parent = NULL);
  virtual ~KLFSideWidget();

  /** \brief returns TRUE if this side widget is currently visible
   *
   * This is not necessarily the same as QWidget::isVisible(), since this widget may have been reparented
   * to some container, which itself is being shown or hidden. */
  bool sideWidgetVisible() const;

  /** \brief returns the current side widget manager type
   *
   * Returns the type of side widget manager used here. This is the key used in the KLFSideWidgetManagerBase
   * factory. Standard keys are "ShowHide", "Float", and "Drawer" (Mac only).
   * But you may create your own.
   *
   * See also \ref KLFSideWidgetManagerBase.
   */
  QString sideWidgetManagerType() const;

  /** \brief returns the instance of the side widget manager used for this side widget
   *
   * See \ref KLFSideWidgetManagerBase.
   */
  KLFSideWidgetManagerBase * sideWidgetManager();

signals:
  /** \brief emitted whenever this side widget is shown or hidden
   *
   * This is also emitted when the side widget manager has changed. */
  void sideWidgetShown(bool shown);
  /** \brief emitted whenver the manager associated to this side widget has changed.
   *
   * See \ref setSideWidgetManager() and \ref KLFSideWidgetManagerBase.
   */
  void sideWidgetManagerTypeChanged(const QString& managerType);

public slots:
  /** \brief show or hide the side widget.
   *
   * If TRUE (resp. FALSE), then shows (resp. hides) the side widget using the current side widget manager.
   *
   * If no side widget manager was set, a warning is issued and a "Float" side widget manager is created.
   */
  void showSideWidget(bool show = true);
  /** \brief hide or show the side widget.
   *
   * Exactly the same as <code>showSideWidget(!hide)</code>. May be useful for signals/slots.
   */
  void hideSideWidget(bool hide = true) { showSideWidget(!hide); }
  /** \brief toggles the show/hide state of this side widget.
   *
   * Exactly the same as <code>showSideWidget(!sideWidgetVisible())</code>.
   */
  void toggleSideWidget() { showSideWidget(!sideWidgetVisible()); }

  /** Instantiate a new side widget manager of the given type and associate it to this widget. */
  void setSideWidgetManager(SideWidgetManager mtype);
  /** Instantiate a new side widget manager of the given type and associate it to this widget.
   * This function allows for customized side widget managers, for which you have registered a factory.
   * See \ref KLFSideWidgetManagerBase. */
  void setSideWidgetManager(const QString& mtype);

  /** Useful to debug the behavior of show/hide in Qt Designer preview. Just connect anything to this slot
   * to enable full show/hide functionality of this widget. (Normally, in Qt Designer, no show/hide process
   * is taking place, to avoid unadvertently hiding the widget for good!)
   */
  void debug_unlock_qtdesigner();

private:

  KLF_DECLARE_PRIVATE(KLFSideWidget) ;

  friend class KLFSideWidgetDesPlugin;
  bool _inqtdesigner;
};




#endif
