/***************************************************************************
 *   Copyright (C) 2010 by David Edmundson <kde@davidedmundson.co.uk>      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QWebView>
#include "chatwindowstyle.h"
#include "telepathychatmessageinfo.h"
#include "telepathychatinfo.h"
#include <KEmoticons>



class AdiumThemeView : public QWebView
{
    Q_OBJECT
public:
    explicit AdiumThemeView(QWidget *parent = 0);
    void initialise(const TelepathyChatInfo&);

    //override various parts loaded from the config file.
    //note that the following will clear the contents of the chat window.

    const QString variantPath() const;
    const QString variantName() const;
    void setVariant(const QString& variant);

    ChatWindowStyle *chatStyle() const;
    void setChatStyle(ChatWindowStyle* chatStyle);

    bool isHeaderDisplayed() const;
    void setHeaderDisplayed(bool);
    /* .. font, backgrounds, everything else.*/


public slots:
    void addMessage(const TelepathyChatMessageInfo &message);

private:
    ChatWindowStyle* m_chatStyle;
    QString m_variantPath;
    QString m_variantName;
    KEmoticons m_emoticons;
    QString replaceHeaderKeywords(QString htmlTemplate, const TelepathyChatInfo&);
    //QString replaceMessageKeywords(QString htmlTemplate, const TelepathyChatMessageInfo&);

    QString formatTime(const QString&, const QDateTime&);

    TelepathyChatInfo m_chatInfo;

    QString m_lastSender;
    bool m_displayHeader;

    void appendNewMessage(QString&);
    void appendNextMessage(QString&);

    bool m_webInspector;
};

#endif // CHATVIEW_H