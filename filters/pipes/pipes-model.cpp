/*
 *    Copyright (C) 2012  Lasath Fernando <kde@lasath.org>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "pipes-model.h"

PipesModel::PipesModel(PipesPrefs *prefs) :
    m_prefs(prefs)
{
    m_columnNames << QLatin1String("Command") << QLatin1String("Direction") << QLatin1String("Format");
}

QVariant PipesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_columnNames[section];
    }

    return QVariant();
}

QVariant PipesModel::data(const QModelIndex &index, int role) const
{
    return QLatin1String("Yo Mamma");
}

int PipesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    return m_columnNames.length();
}

int PipesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED (parent);
    return m_prefs->pipeList().size();
}

