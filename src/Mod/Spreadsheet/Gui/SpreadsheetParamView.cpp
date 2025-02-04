/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015-2016        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QPalette>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QTextDocument>
#endif

#include <App/DocumentObject.h>
#include <App/Range.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>

#include <Mod/Spreadsheet/App/Sheet.h>

//#include "LineEdit.h"
#include "SpreadsheetDelegate.h"
#include "SpreadsheetParamView.h"
//#include "ZoomableView.h"
#include "qtcolorpicker.h"
#include "ui_Params.h"



using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace Gui;
using namespace App;
namespace sp = std::placeholders;

/* TRANSLATOR SpreadsheetGui::SheetParamView */

TYPESYSTEM_SOURCE_ABSTRACT(SpreadsheetGui::SheetParamView, Gui::MDIView)

SheetParamView::SheetParamView(Gui::Document* pcDocument, App::DocumentObject* docObj, QWidget* parent)
    : MDIView(pcDocument, parent)
    , sheet(static_cast<Sheet*>(docObj))
{
    // Set up ui

    model = new SheetModel(static_cast<Sheet*>(docObj));

    ui = new Ui::Params();
    QWidget* w = new QWidget(this);
    ui->setupUi(w);
    setCentralWidget(w);

    // new ZoomableView(ui); // TODO FIXME

    delegate = new SpreadsheetDelegate(sheet);
    ui->cells->setModel(model);
    ui->cells->setItemDelegate(delegate);
    ui->cells->setSheet(sheet);

    // Connect signals
    connect(ui->cells->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &SheetParamView::currentChanged);

    /*connect(dynamic_cast<SheetViewHeader*>(ui->cells->horizontalHeader()),
            &SheetViewHeader::resizeFinished,
            this,
            &SheetParamView::columnResizeFinished);*/
    connect(ui->cells->horizontalHeader(),
            &QHeaderView::sectionResized,
            this,
            &SheetParamView::columnResized);

    /*connect(dynamic_cast<SheetViewHeader*>(ui->cells->verticalHeader()),
            &SheetViewHeader::resizeFinished,
            this,
            &SheetParamView::rowResizeFinished);*/
    connect(ui->cells->verticalHeader(),
            &QHeaderView::sectionResized,
            this,
            &SheetParamView::rowResized);

    connect(delegate,
            &SpreadsheetDelegate::finishedWithKey,
            this,
            &SheetParamView::editingFinishedWithKey);

    // NOLINTBEGIN
    columnWidthChangedConnection = sheet->columnWidthChanged.connect(
        std::bind(&SheetParamView::resizeColumn, this, sp::_1, sp::_2));
    rowHeightChangedConnection =
        sheet->rowHeightChanged.connect(std::bind(&SheetParamView::resizeRow, this, sp::_1, sp::_2));
    // NOLINTEND

    connect(model, &QAbstractItemModel::dataChanged, this, &SheetParamView::modelUpdated);

    QPalette palette = ui->cells->palette();
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::Text, QColor(0, 0, 0));
    ui->cells->setPalette(palette);

    QList<QtColorPicker*> bgList = Gui::getMainWindow()->findChildren<QtColorPicker*>(
        QString::fromLatin1("Spreadsheet_BackgroundColor"));
    if (!bgList.empty()) {
        bgList[0]->setCurrentColor(palette.color(QPalette::Base));
    }

    QList<QtColorPicker*> fgList = Gui::getMainWindow()->findChildren<QtColorPicker*>(
        QString::fromLatin1("Spreadsheet_ForegroundColor"));
    if (!fgList.empty()) {
        fgList[0]->setCurrentColor(palette.color(QPalette::Text));
    }
}

SheetParamView::~SheetParamView()
{
    Gui::Application::Instance->detachView(this);
    delete ui;
    delete model;
    delete delegate;
}

bool SheetParamView::onMsg(const char* pMsg, const char**)
{
    if (strcmp("Undo", pMsg) == 0) {
        getGuiDocument()->undo(1);
        App::Document* doc = getAppDocument();
        if (doc) {
            doc->recompute();
        }
        return true;
    }
    else if (strcmp("Redo", pMsg) == 0) {
        getGuiDocument()->redo(1);
        App::Document* doc = getAppDocument();
        if (doc) {
            doc->recompute();
        }
        return true;
    }
    else if (strcmp("Save", pMsg) == 0) {
        getGuiDocument()->save();
        return true;
    }
    else if (strcmp("SaveAs", pMsg) == 0) {
        getGuiDocument()->saveAs();
        return true;
    }
    else if (strcmp("Std_Delete", pMsg) == 0) {
        std::vector<Range> ranges = selectedRanges();
        if (sheet->hasCell(ranges)) {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Clear cell(s)"));
            std::vector<Range>::const_iterator i = ranges.begin();
            for (; i != ranges.end(); ++i) {
                FCMD_OBJ_CMD(sheet, "clear('" << i->rangeString() << "')");
            }
            Gui::Command::commitCommand();
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        return true;
    }
    else if (strcmp("Cut", pMsg) == 0) {
        ui->cells->cutSelection();
        return true;
    }
    else if (strcmp("Copy", pMsg) == 0) {
        ui->cells->copySelection();
        return true;
    }
    else if (strcmp("Paste", pMsg) == 0) {
        ui->cells->pasteClipboard();
        return true;
    }
    else {
        return false;
    }
}

bool SheetParamView::onHasMsg(const char* pMsg) const
{
    if (strcmp("Undo", pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableUndos() > 0;
    }
    if (strcmp("Redo", pMsg) == 0) {
        App::Document* doc = getAppDocument();
        return doc && doc->getAvailableRedos() > 0;
    }
    if (strcmp("Save", pMsg) == 0) {
        return true;
    }
    if (strcmp("SaveAs", pMsg) == 0) {
        return true;
    }
    if (strcmp("Cut", pMsg) == 0) {
        return true;
    }
    if (strcmp("Copy", pMsg) == 0) {
        return true;
    }
    if (strcmp("Paste", pMsg) == 0) {
        return true;
    }
    if (strcmp(pMsg, "Print") == 0) {
        return true;
    }
    if (strcmp(pMsg, "PrintPreview") == 0) {
        return true;
    }
    if (strcmp(pMsg, "PrintPdf") == 0) {
        return true;
    }
    else if (strcmp("AllowsOverlayOnHover", pMsg) == 0) {
        return true;
    }

    return false;
}

/**
 * Shows the printer dialog.
 */
void SheetParamView::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setFullPage(true);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        print(&printer);
    }
}

void SheetParamView::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setPageOrientation(QPageLayout::Landscape);
    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg,
            &QPrintPreviewDialog::paintRequested,
            this,
            qOverload<QPrinter*>(&SheetParamView::print));
    dlg.exec();
}

void SheetParamView::print(QPrinter* printer)
{
#if 0
    ui->cells->render(printer);
#endif
    std::unique_ptr<QTextDocument> document = std::make_unique<QTextDocument>();
    document->setHtml(ui->cells->toHtml());
    document->print(printer);
}

/**
 * Prints the document into a Pdf file.
 */
void SheetParamView::printPdf()
{
    QString filename =
        FileDialog::getSaveFileName(this,
                                    tr("Export PDF"),
                                    QString(),
                                    QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));
    if (!filename.isEmpty()) {
        QPrinter printer(QPrinter::ScreenResolution);
        // setPdfVersion sets the printied PDF Version to comply with PDF/A-1b, more details under:
        // https://www.kdab.com/creating-pdfa-documents-qt/
        printer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
        printer.setPageOrientation(QPageLayout::Landscape);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        print(&printer);
    }
}

void SheetParamView::setCurrentCell(QString str)
{
    Q_UNUSED(str);
    updateContentLine();
    updateAliasLine();
}

void SheetParamView::updateContentLine()
{
    QModelIndex i = ui->cells->currentIndex();

    if (i.isValid()) {
        std::string str;
        if (const auto* cell = sheet->getCell(CellAddress(i.row(), i.column()))) {
            (void)cell->getStringContent(str);
        }
    }
}

void SheetParamView::updateAliasLine()
{
    QModelIndex i = ui->cells->currentIndex();

    if (i.isValid()) {
        std::string str;
        if (const auto* cell = sheet->getCell(CellAddress(i.row(), i.column()))) {
            (void)cell->getAlias(str);
        }
    }
}

void SheetParamView::columnResizeFinished()
{
    if (newColumnSizes.empty()) {
        return;
    }

    blockSignals(true);
    for (auto& v : newColumnSizes) {
        sheet->setColumnWidth(v.first, v.second);
    }
    blockSignals(false);
    newColumnSizes.clear();
}

void SheetParamView::rowResizeFinished()
{
    if (newRowSizes.empty()) {
        return;
    }

    blockSignals(true);
    for (auto& v : newRowSizes) {
        sheet->setRowHeight(v.first, v.second);
    }
    blockSignals(false);
    newRowSizes.clear();
}

void SheetParamView::modelUpdated(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    const QModelIndex& current = ui->cells->currentIndex();

    if (current < topLeft || bottomRight < current) {
        return;
    }

    updateContentLine();
    updateAliasLine();
}

void SheetParamView::columnResized(int col, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    newColumnSizes[col] = newSize;
}

void SheetParamView::rowResized(int row, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    newRowSizes[row] = newSize;
}

void SheetParamView::resizeColumn(int col, int newSize)
{
    if (ui->cells->horizontalHeader()->sectionSize(col) != newSize) {
        ui->cells->setColumnWidth(col, newSize);
    }
}

void SheetParamView::resizeRow(int col, int newSize)
{
    if (ui->cells->verticalHeader()->sectionSize(col) != newSize) {
        ui->cells->setRowHeight(col, newSize);
    }
}

void SheetParamView::editingFinishedWithKey(int key, Qt::KeyboardModifiers modifiers)
{
    QModelIndex i = ui->cells->currentIndex();

    if (i.isValid()) {
        ui->cells->finishEditWithMove(key, modifiers);
    }
}

void SheetParamView::confirmAliasChanged(const QString& text)
{
    bool aliasOkay = true;

    if (text.length() != 0 && !sheet->isValidAlias(text.toStdString())) {
        aliasOkay = false;
    }

    QModelIndex i = ui->cells->currentIndex();
    if (const auto* cell = sheet->getNewCell(CellAddress(i.row(), i.column()))) {
        if (!aliasOkay) {
            // do not show error message if failure to set new alias is because it is already the
            // same string
            std::string current_alias;
            (void)cell->getAlias(current_alias);
            if (text != QString::fromUtf8(current_alias.c_str())) {
                Base::Console().Error("Unable to set alias: %s\n", text.toStdString().c_str());
            }
        }
        else {
            std::string address = CellAddress(i.row(), i.column()).toString();
            Gui::cmdAppObjectArgs(sheet, "setAlias('%s', '%s')", address, text.toStdString());
            Gui::cmdAppDocument(sheet->getDocument(), "recompute()");
            ui->cells->setFocus();
        }
    }
}


void SheetParamView::confirmContentChanged(const QString& text)
{
    QModelIndex i = ui->cells->currentIndex();
    ui->cells->model()->setData(i, QVariant(text), Qt::EditRole);
    ui->cells->setFocus();
}

void SheetParamView::aliasChanged(const QString& text)
{
}

void SheetParamView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    updateContentLine();
    updateAliasLine();
}

void SheetParamView::updateCell(const App::Property* prop)
{
    try {
        if (prop == &sheet->Label) {
            QString cap = QString::fromUtf8(sheet->Label.getValue());
            setWindowTitle(cap);
        }
        CellAddress address;

        if (!sheet->getCellAddress(prop, address)) {
            return;
        }

        if (currentIndex().row() == address.row() && currentIndex().column() == address.col()) {
            updateContentLine();
            updateAliasLine();
        }
    }
    catch (...) {
        // Property is not a cell
        return;
    }
}

std::vector<Range> SheetParamView::selectedRanges() const
{
    return ui->cells->selectedRanges();
}

QModelIndexList SheetParamView::selectedIndexes() const
{
    return ui->cells->selectionModel()->selectedIndexes();
}

QModelIndexList SheetParamView::selectedIndexesRaw() const
{
    return ui->cells->selectedIndexesRaw();
}

void SpreadsheetGui::SheetParamView::select(App::CellAddress cell,
                                       QItemSelectionModel::SelectionFlags flags)
{
    ui->cells->selectionModel()->select(model->index(cell.row(), cell.col()), flags);
}

void SpreadsheetGui::SheetParamView::select(App::CellAddress topLeft,
                                       App::CellAddress bottomRight,
                                       QItemSelectionModel::SelectionFlags flags)
{
    ui->cells->selectionModel()->select(
        QItemSelection(model->index(topLeft.row(), topLeft.col()),
                       model->index(bottomRight.row(), bottomRight.col())),
        flags);
}

void SheetParamView::deleteSelection()
{
    ui->cells->deleteSelection();
}

QModelIndex SheetParamView::currentIndex() const
{
    return ui->cells->currentIndex();
}

void SpreadsheetGui::SheetParamView::setCurrentIndex(App::CellAddress cell) const
{
    ui->cells->setCurrentIndex(model->index(cell.row(), cell.col()));
}

PyObject* SheetParamView::getPyObject()
{
    return NULL;
}

void SheetParamView::deleteSelf()
{
    Gui::MDIView::deleteSelf();
}

// ----------------------------------------------------------

#include "moc_SpreadsheetView.cpp"
