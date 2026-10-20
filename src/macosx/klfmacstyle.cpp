// macOS presentation for the existing equation editor and export actions.
// This file is part of KLatexFormula, licensed under GPL version 2 or later.

#include <QtWidgets>
#include <ui_klfmainwin.h>

#include "klfmainwin.h"

// Keep the original Illustrator paths, recoloring only their ink for dark chrome.
QIcon klf_mac_button_icon(const QString& resource)
{
  QIcon original(resource), result;
  for (int scale : {1, 2, 3}) {
    QPixmap image = original.pixmap(QSize(22 * scale, 22 * scale));
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), QColor("#e4e4e4"));
    painter.end();
    image.setDevicePixelRatio(scale);
    result.addPixmap(image, QIcon::Normal);
    result.addPixmap(image, QIcon::Active);
    result.addPixmap(image, QIcon::Selected);
  }
  return result;
}

namespace {
void textButton(QPushButton *button, const QString& text)
{
  button->setText(text);
  button->setMinimumHeight(24);
  button->setMaximumHeight(QWIDGETSIZE_MAX);
  button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  button->setAttribute(Qt::WA_MacShowFocusRect, false);
}
void drawnButton(QAbstractButton *button, const QString& name, const QString& resource,
                 bool label = false)
{
  button->setAccessibleName(name);
  button->setText(label ? name : QString());
  button->setIcon(klf_mac_button_icon(resource));
  button->setIconSize(QSize(22, 22));
  button->setMinimumHeight(24);
  button->setAttribute(Qt::WA_MacShowFocusRect, false);
}
}

void KLFMainWin::setupMacAppearance()
{
  const QFont systemFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
  setFont(systemFont);
  u->lyt_KLFMainWin->setContentsMargins(6, 6, 6, 6);
  u->lyt_KLFMainWin->setSpacing(6);
  u->lyt_frmMain->setContentsMargins(0, 0, 0, 0);
  u->lyt_frmMain->setSpacing(4);
  u->btnQuit->hide(); // macOS provides Quit in the application menu and Cmd-Q.

  setWindowIcon(QIcon(":/macosx/app-icon.svg"));
  drawnButton(u->btnSymbols, tr("Symbols"), ":/pics/symbols.svg");
  drawnButton(u->btnLibrary, tr("Library"), ":/pics/library.svg");
  drawnButton(u->btnExpand, tr("Options"), ":/pics/switchexpanded.svg");

  // Wrap the existing editor, preserving its shortcuts, highlighting and drops.
  QWidget *editorPanel = new QWidget(u->splitMain);
  editorPanel->setObjectName("macEditorPanel");
  QVBoxLayout *editorLayout = new QVBoxLayout(editorPanel);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);
  u->txtLatex->setParent(editorPanel);
  u->txtLatex->setFrameShape(QFrame::NoFrame);
  u->txtLatex->setMinimumHeight(64);
  u->txtLatex->setPlaceholderText(tr("Enter LaTeX"));
  u->txtLatex->setFont(systemFont);
  u->txtPreamble->setFont(systemFont);
  editorLayout->addWidget(u->txtLatex);
  u->splitMain->insertWidget(0, editorPanel);

  drawnButton(u->btnClearAll, tr("Clear"), ":/pics/clear.svg");
  drawnButton(u->btnClearLatex, tr("Clear"), ":/pics/clearedit.svg");
  drawnButton(u->btnEvaluate, tr("Render"), ":/pics/evaluate.svg", true);
  u->btnEvaluate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  u->lytMiddle->setSpacing(4);
  u->lytMiddle->setContentsMargins(0, 0, 0, 4);
  u->vlyt_OutputFrame->setSpacing(0);

  // Replace the vertical export stack with a single, labeled action bar.
  while (QLayoutItem *item = u->lyt_frmOutput->takeAt(0)) {
    delete item; // Layout items do not own the widgets.
  }
  u->lyt_frmOutput->setContentsMargins(0, 0, 0, 0);
  u->lyt_frmOutput->setSpacing(4);
  u->lyt_frmOutput->setColumnStretch(0, 1);
  u->lyt_frmOutput->setColumnStretch(1, 0);
  u->lblOutput->setMinimumSize(260, 80);
  u->lblOutput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  u->lblOutput->setMargin(4);
  u->lblOutput->setAccessibleName(tr("Rendered equation preview"));
  u->lblOutput->setProperty("themedSurface", true);
  u->lyt_frmOutput->addWidget(u->lblOutput, 0, 0);
  u->lyt_frmOutput->setRowStretch(0, 1);

  QHBoxLayout *exportLayout = new QHBoxLayout;
  exportLayout->setSpacing(4);
  textButton(u->btnDrag, tr("Drag"));
  textButton(u->btnCopy, tr("Copy"));
  textButton(u->btnSave, tr("Save…"));
  exportLayout->addWidget(u->btnCopy);
  exportLayout->addWidget(u->btnSave);
  exportLayout->addWidget(u->btnDrag);
  exportLayout->addStretch();
  u->lyt_wForBigPreviewButton->removeWidget(u->btnShowBigPreview);
  u->lyt_wForBigPreviewButton->removeWidget(u->btnSetExportProfile);
  drawnButton(u->btnShowBigPreview, tr("Zoom"), ":/pics/showpreview.svg");
  exportLayout->addWidget(u->btnShowBigPreview);
  drawnButton(u->btnSetExportProfile, tr("Format"), ":/pics/exportprofile.svg");
  u->btnSetExportProfile->setToolButtonStyle(Qt::ToolButtonIconOnly);
  u->btnSetExportProfile->setMinimumHeight(24);
  exportLayout->addWidget(u->btnSetExportProfile);
  drawnButton(u->btnHelp, tr("Help"), ":/pics/help.svg");
  u->wForBigPreviewButton->hide();
  u->lyt_frmOutput->addLayout(exportLayout, 1, 0);

  // NSDrawer predates modern macOS; keep options in the resizable main window.
  if (u->frmDetails->parentWidget() != u->wCentralWidget) {
    u->frmDetails->setParent(u->wCentralWidget);
    u->lyt_KLFMainWin->addWidget(u->frmDetails);
  }
  u->frmDetails->setSideWidgetManager(QLatin1String("ShowHide"));
  u->frmDetails->setMinimumWidth(280);
  u->frmDetails->setMaximumWidth(320);
  u->lyt_KLFMainWin->setStretch(0, 1);
  u->lyt_KLFMainWin->setStretch(1, 0);
  for (QFrame *frame : u->frmDetails->findChildren<QFrame *>()) {
    if (frame->frameShape() == QFrame::HLine || frame->frameShape() == QFrame::VLine)
      frame->hide();
  }
  for (QComboBox *combo : u->frmDetails->findChildren<QComboBox *>()) {
    // Avoid the native popup's independently chosen light selection palette.
    combo->setView(new QListView(combo));
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(10);
  }
  u->splitMain->setChildrenCollapsible(false);
  u->splitMain->setHandleWidth(4);
  u->splitMain->setStretchFactor(0, 1);
  u->splitMain->setStretchFactor(1, 1);
  u->splitMain->setSizes(QList<int>() << 140 << 200);
  setMinimumSize(380, 280);
  resize(400, 360);
  refreshMacAppearance();

  extern int klf_mac_modern_window(QWidget *);
  const int titlebar = klf_mac_modern_window(this);
  u->lyt_KLFMainWin->setContentsMargins(6, 6 + titlebar, 6, 6);
  setMinimumSize(380, 280 + titlebar);
  resize(400, 360 + titlebar);
  // Cocoa/Qt finishes configuring the title bar when the window is shown.
  // Reapply after that work so its fill stays continuous with the content.
  installEventFilter(this);
}

void KLFMainWin::refreshMacAppearance()
{
  const bool dark = true;
  const QString background = dark ? "#292929" : "#f1f1f1";
  const QString panel = dark ? "#2d2d2d" : "#ffffff";
  const QString text = dark ? "#eeeeee" : "#202020";
  const QString muted = dark ? "#b5b5b5" : "#606060";
  const QString border = dark ? "#787878" : "#999999";
  const QString softBorder = dark ? "#505050" : "#c8c8c8";
  const QString button = dark ? "#3c3c3c" : "#fafafa";
  const QString hover = dark ? "#494949" : "#e8e8e8";
  const QString pressed = dark ? "#585858" : "#d7d7d7";
  const QString disabled = dark ? "#a6a6a6" : "#6a6a6a";
  const QString disabledBackground = dark ? "#303030" : "#e5e5e5";
  const QString focus = dark ? "#cccccc" : "#555555";
  const QString selection = dark ? "#686868" : "#d0d0d0";
  QPalette neutralPalette = QApplication::palette();
  neutralPalette.setColor(QPalette::Window, QColor(background));
  neutralPalette.setColor(QPalette::WindowText, QColor(text));
  neutralPalette.setColor(QPalette::Base, QColor(panel));
  neutralPalette.setColor(QPalette::AlternateBase, QColor(background));
  neutralPalette.setColor(QPalette::Text, QColor(text));
  neutralPalette.setColor(QPalette::Button, QColor(button));
  neutralPalette.setColor(QPalette::ButtonText, QColor(text));
  neutralPalette.setColor(QPalette::ToolTipBase, QColor(panel));
  neutralPalette.setColor(QPalette::ToolTipText, QColor(text));
  neutralPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(disabled));
  neutralPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(disabled));
  neutralPalette.setColor(QPalette::Highlight, QColor(selection));
  neutralPalette.setColor(QPalette::HighlightedText, QColor(text));
  neutralPalette.setColor(QPalette::Link, QColor(text));
  neutralPalette.setColor(QPalette::LinkVisited, QColor(muted));
  if (qApp->palette() != neutralPalette)
    qApp->setPalette(neutralPalette);
  // Legacy syntax colors were chosen for a white editor. Keep a copy of the
  // user's formats and adapt their foregrounds when the system uses dark mode.
  for (KLFLatexEdit *editor : {u->txtLatex, u->txtPreamble}) {
    KLFLatexSyntaxHighlighter *highlighter = editor->syntaxHighlighter();
    const QStringList formats = {"fmtKeyword", "fmtComment", "fmtParenMatch",
                                 "fmtParenMismatch", "fmtLonelyParen"};
    for (const QString& format : formats) {
      const QByteArray name = format.toLatin1();
      const QByteArray saved = "macOriginal_" + name;
      if (!highlighter->property(saved).isValid())
        highlighter->setProperty(saved, highlighter->property(name));
      QTextCharFormat value = highlighter->property(saved).value<QTextFormat>().toCharFormat();
      value.setForeground(QColor(format == "fmtComment" ? muted : text));
      if (format == "fmtParenMatch" || format == "fmtParenMismatch")
        value.setBackground(QColor(pressed));
      if (format == "fmtParenMismatch" || format == "fmtLonelyParen") {
        value.setFontUnderline(true);
        value.setUnderlineColor(QColor(text));
      }
      highlighter->setProperty(name, QVariant::fromValue(QTextFormat(value)));
    }
    highlighter->rehighlight();
  }
  QString css = QString::fromLatin1(R"CSS(
    QMainWindow#KLFMainWin {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #292929, stop:0.3 #292929, stop:1 #202020); color: @text;
    }
    KLFLatexSymbols, QWidget#frmSymbolList { background: @background; color: @text; }
    QDialog#KLFProgErr { background: @background; color: @text; }
    QLabel { color: @text; }
    QTextEdit, QPlainTextEdit {
      background: #333333; color: @text; border: none;
      border-radius: 8px; padding: 6px;
      selection-background-color: @selection; selection-color: @text;
    }
    QWidget#macEditorPanel, QFrame#frmOutput {
      background: transparent; border: none;
    }
    QTextEdit#txtLatex {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #383838, stop:1 #303030);
      color: @text; border: 1px solid transparent;
      border-radius: 10px; padding: 6px;
      selection-background-color: @selection; selection-color: @text;
    }
    QTextEdit#txtLatex:focus {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #404040, stop:1 #333333);
    }
    QLabel#lblOutput {
      background: #ffffff; color: #202020; border: 1px solid transparent;
      border-radius: 10px;
    }
    QLabel#lblOutput[realTimeLatexError="true"] {
      background: @hover; color: @text; border: 1px solid transparent;
    }
    QPushButton, QToolButton {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #444444, stop:1 #353535);
      color: @text; border: 1px solid transparent;
      border-radius: 3px; padding: 0px 6px; min-height: 22px;
      outline: none;
    }
    QPushButton:hover, QToolButton:hover { background: @hover; }
    QPushButton:pressed, QToolButton:pressed { background: @pressed; }
    QPushButton:checked, QToolButton:checked { background: @pressed; color: @text; }
    QPushButton:focus, QToolButton:focus { background: #606060; }
    QPushButton:disabled, QToolButton:disabled, QPushButton#btnEvaluate:disabled {
      color: @disabled; background: @disabledBackground; border: 1px solid transparent;
    }
    KLFPixmapButton {
      background: #ffffff; color: #202020; border: 1px solid transparent;
      border-radius: 4px; padding: 2px; min-height: 24px;
    }
    KLFPixmapButton:hover { background: #dddddd; }
    KLFPixmapButton:pressed { background: #d5d5d5; }
    QTabWidget::pane {
      background: #303030; border: none; border-radius: 8px;
      top: -1px;
    }
    QTabBar::tab {
      background: @button; color: @text; border: 1px solid transparent;
      border-radius: 4px; padding: 4px 6px; margin: 0px 2px 4px 0px;
    }
    QTabBar::tab:hover:!selected { background: @hover; color: @text; }
    QTabBar::tab:selected, QTabBar::tab:selected:!active {
      background: #d0d0d0; color: #202020; border: 1px solid transparent;
    }
    QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit#txtPreamble {
      background: #404040; color: @text; border: 1px solid transparent;
      border-radius: 6px; padding: 2px 5px; min-height: 20px;
      selection-background-color: @selection; selection-color: @text;
    }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QTextEdit#txtPreamble:focus {
      background: #4d4d4d;
    }
    QComboBox QLineEdit { border: none; padding: 0px; min-height: 0px; background: transparent; }
    QComboBox::drop-down { border: none; width: 24px; }
    QComboBox::down-arrow { image: url(@chevron); width: 12px; height: 12px; }
    QComboBox QAbstractItemView { background: @panel; color: @text; selection-background-color: @selection; selection-color: @text; }
    QComboBox QAbstractItemView::item:selected { background: @selection; color: @text; }
    QMenu { background: @panel; color: @text; border: none; }
    QMenu::item:selected { background: @selection; color: @text; }
    QToolButton::menu-indicator, QPushButton::menu-indicator {
      image: url(@chevron); width: 10px; height: 10px;
      subcontrol-position: right center; subcontrol-origin: padding;
    }
    QGroupBox {
      color: @text; background: #373737; border: none; border-radius: 8px;
      margin-top: 8px; padding: 8px 4px 4px 4px;
    }
    QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0px 4px; }
    /* Keep the scroll models and wheel/key handling, without visible rails,
       arrow buttons, thumbs or reserved space in any application window. */
    QScrollBar { background: transparent; border: none; margin: 0px; padding: 0px; }
    QScrollBar:vertical { width: 0px; min-width: 0px; max-width: 0px; }
    QScrollBar:horizontal { height: 0px; min-height: 0px; max-height: 0px; }
    QScrollBar::handle { background: transparent; border: none; min-width: 0px; min-height: 0px; }
    QScrollBar::add-line, QScrollBar::sub-line {
      background: transparent; border: none; width: 0px; height: 0px;
    }
    QScrollBar::add-page, QScrollBar::sub-page { background: transparent; border: none; }
    QSplitter::handle { background: transparent; }
    QToolTip { background: @panel; color: @text; border: none; padding: 6px; }
  )CSS");
  const QList<QPair<QString, QString> > tokens = {
    {"@background", background}, {"@panel", panel}, {"@text", text},
    {"@muted", muted}, {"@border", border}, {"@softBorder", softBorder},
    {"@button", button}, {"@hover", hover}, {"@pressed", pressed},
    {"@focus", focus}, {"@selection", selection},
    {"@chevron", dark ? ":/pics/macos-chevron-dark.svg" : ":/pics/macos-chevron-light.svg"},
    {"@disabledBackground", disabledBackground}, {"@disabled", disabled}
  };
  for (const auto& token : tokens) css.replace(token.first, token.second);
  // Auxiliary windows are independent top-level widgets on macOS. Apply the
  // same controls there, including white tiles for black symbol thumbnails.
  if (qApp->styleSheet() != css)
    qApp->setStyleSheet(css);
}
