/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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

#ifndef SpreadsheetParamView_H
#define SpreadsheetParamView_H

#include <QHeaderView>

#include <Gui/MDIView.h>
#include <Gui/MDIViewPy.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "SheetModel.h"


class QSlider;
class QAction;
class QActionGroup;
class QPopupMenu;
class QToolBar;

namespace App
{
class DocumentObject;
class Property;
}  // namespace App

namespace Ui
{
class Params;
}

class QTableWidgetItem;

namespace SpreadsheetGui
{

class SpreadsheetDelegate;

class SpreadsheetGuiExport SheetParamView: public Gui::MDIView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SheetParamView(Gui::Document* pcDocument, App::DocumentObject* docObj, QWidget* parent);
    ~SheetParamView() override;

    const char* getName() const override
    {
        return "SheetParamView";
    }

    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;

    /** @name Printing */
    //@{
    void print() override;
    void printPdf() override;
    void printPreview() override;
    void print(QPrinter*) override;
    //@}

    void updateCell(const App::Property* prop);

    Spreadsheet::Sheet* getSheet()
    {
        return sheet;
    }

    std::vector<App::Range> selectedRanges() const;

    QModelIndexList selectedIndexes() const;
    QModelIndexList selectedIndexesRaw() const;

    void select(App::CellAddress cell, QItemSelectionModel::SelectionFlags flags);

    void select(App::CellAddress topLeft,
                App::CellAddress bottomRight,
                QItemSelectionModel::SelectionFlags flags);

    QModelIndex currentIndex() const;

    void setCurrentIndex(App::CellAddress cell) const;

    void deleteSelection();

    PyObject* getPyObject() override;

    void deleteSelf() override;

protected Q_SLOTS:
    void editingFinishedWithKey(int key, Qt::KeyboardModifiers modifiers);
    void confirmAliasChanged(const QString& text);
    void aliasChanged(const QString& text);
    void confirmContentChanged(const QString& text);
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void columnResized(int col, int oldSize, int newSize);
    void rowResized(int row, int oldSize, int newSize);
    void columnResizeFinished();
    void rowResizeFinished();
    void modelUpdated(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected:
    void updateContentLine();
    void updateAliasLine();
    void setCurrentCell(QString str);
    void resizeColumn(int col, int newSize);
    void resizeRow(int col, int newSize);

    Ui::Params* ui;
    Spreadsheet::Sheet* sheet;
    SpreadsheetDelegate* delegate;
    SheetModel* model;
    boost::signals2::scoped_connection columnWidthChangedConnection;
    boost::signals2::scoped_connection rowHeightChangedConnection;
    boost::signals2::scoped_connection positionChangedConnection;

    std::map<int, int> newColumnSizes;
    std::map<int, int> newRowSizes;
};

}  // namespace SpreadsheetGui

#endif  // SpreadsheetParamView_H
