/*
 *    Copyright (C) 2014  Marcin Ziemiński <zieminn@gmail.com>
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

#ifndef OTR_FILTER_H
#define OTR_FILTER_H

#include <KTp/abstract-message-filter.h>

class OTRFilter : public KTp::AbstractMessageFilter
{
    Q_OBJECT

public:
    OTRFilter(QObject *parent, const QVariantList &);
    virtual void filterMessage(KTp::Message &message, const KTp::MessageContext &context);
};

#endif // OTR_FILTER_H
