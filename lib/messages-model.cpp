/*
    Copyright (C) 2011  Lasath Fernando <kde@lasath.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "messages-model.h"

#include <KDebug>
#include <TelepathyQt/ReceivedMessage>

class MessageItem {
public:
    QString user;
    QString text;
    QDateTime time;
    QString id;

    //FIXME : replace with Tp::ChannelTextMessageType
    enum MessageType {
        Incoming,
        Outgoing,
        Status
    } type;

    MessageItem(QString user, QString text, QDateTime time, MessageType type, QString messageId)
     : user(user), text(text), time(time), type(type), id(messageId)
    {
        if(this->text.endsWith(QLatin1String("\n"))) {
            this->text.chop(1);
        }
    }
};

class MessagesModel::ConversationModelPrivate {
public:
    Tp::TextChannelPtr textChannel;
    QList<MessageItem> messages;
    bool visible;
};

MessagesModel::MessagesModel(QObject* parent):
    QAbstractListModel(parent),
    d(new ConversationModelPrivate)
{
    kDebug();

    QHash<int, QByteArray> roles;
    roles[UserRole] = "user";
    roles[TextRole] = "text";
    roles[TimeRole] = "time";
    roles[TypeRole] = "type";
    setRoleNames(roles);

    d->visible = false;
}

Tp::TextChannelPtr MessagesModel::textChannel()
{
    return d->textChannel;
}

bool MessagesModel::verifyPendingOperation ( Tp::PendingOperation* op )
{
    if(op->isError()) {
        kWarning() << op->errorName() << "+" << op->errorMessage();
        return false;
    }
    return true;
}

void MessagesModel::setupChannelSignals(Tp::TextChannelPtr channel)
{
    QObject::connect(channel.constData(),
                    SIGNAL(messageReceived(Tp::ReceivedMessage)),
                    SLOT(onMessageReceived(Tp::ReceivedMessage)));
    QObject::connect(channel.constData(),
                    SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString)),
                    SLOT(onMessageSent(Tp::Message,Tp::MessageSendingFlags,QString)));
}

void MessagesModel::setTextChannel(Tp::TextChannelPtr channel)
{
    kDebug();
    setupChannelSignals(channel);

    if(d->textChannel) {
        removeChannelSignals(channel);
    }
    //FIXME: check messageQue for any lost messages
    d->textChannel = channel;

    Q_EMIT textChannelChanged(channel);

    QList<Tp::ReceivedMessage> que = channel->messageQueue();
    Q_FOREACH(Tp::ReceivedMessage message, que) {
        bool messageAlreadyInModel = false;
        Q_FOREACH(MessageItem current, d->messages) {
            if(current.id == message.messageToken()) {
                messageAlreadyInModel = true;
            }
        }
        if(!messageAlreadyInModel) {
            onMessageReceived(message);
        }
    }
}

void MessagesModel::onMessageReceived(Tp::ReceivedMessage message)
{
    int unreadCount = d->textChannel->messageQueue().size();
    kDebug() << "unreadMessagesCount =" << unreadCount;
    kDebug() << "text =" << message.text();

    if(message.messageType() == Tp::ChannelTextMessageTypeNormal) {
        int length = rowCount();
        beginInsertRows(QModelIndex(), length, length);

        d->messages.append(MessageItem(
                message.sender()->alias(),
                message.text(),
                message.received(),
                MessageItem::Incoming,
                message.messageToken()
        ));

        endInsertRows();

        if(d->visible) {
            acknowledgeAllMessages();
        } else {
            Q_EMIT unreadCountChanged(unreadCount);
        }
    }

}

void MessagesModel::onMessageSent(Tp::Message message, Tp::MessageSendingFlags flags, QString token)
{
    Q_UNUSED(flags);
    Q_UNUSED(token);

    int length = rowCount();
    beginInsertRows(QModelIndex(), length, length);
    kDebug() << "text =" << message.text();

    d->messages.append(MessageItem(
        tr("Me"),   //FIXME : use actual nickname from Tp::AccountPtr
        message.text(),
        message.sent(),
        MessageItem::Outgoing,
        message.messageToken()
    ));

    endInsertRows();
}

QVariant MessagesModel::data(const QModelIndex& index, int role) const
{
    QVariant result;

    if(index.row() < d->messages.size()) {
        MessageItem* requestedData = &d->messages[index.row()];

        switch(role) {
            case UserRole:
                result = requestedData->user;
                break;
            case TextRole:
                result = requestedData->text;
                break;
            case TypeRole:
                result = requestedData->type;
                break;
            case TimeRole:
                result = requestedData->time;
                break;
        };
    } else {
        kError() << "Attempting to access data at invalid index (" << index << ")";
    }

    return result;
}

int MessagesModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return d->messages.size();
}

Tp::PendingSendMessage* MessagesModel::sendNewMessage ( QString message )
{
    Tp::PendingSendMessage* msg = 0;

    if(message.isEmpty()) {
        kWarning() << "Attempting to send empty string";
    } else {
        msg = d->textChannel->send(message);
        connect(msg, SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(verifyPendingOperation(Tp::PendingOperation*)));
    }

    return msg;
}

void MessagesModel::removeChannelSignals(Tp::TextChannelPtr channel)
{
    QObject::disconnect(channel.constData(),
                        SIGNAL(messageReceived(Tp::ReceivedMessage)),
                        this,
                        SLOT(onMessageReceived(Tp::ReceivedMessage))
                    );
    QObject::disconnect(channel.constData(),
                        SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString)),
                        this,
                        SLOT(onMessageSent(Tp::Message,Tp::MessageSendingFlags,QString))
                    );
}

int MessagesModel::unreadCount() const
{
    return d->textChannel->messageQueue().size();
}

void MessagesModel::acknowledgeAllMessages()
{
    QList<Tp::ReceivedMessage> que = d->textChannel->messageQueue();

    kDebug() << "Conversation Visible, Acknowledging " << que.size() << " messages.";

    d->textChannel->acknowledge(que);
    Q_EMIT unreadCountChanged(que.size());
}

void MessagesModel::setVisibleToUser(bool visible)
{
    kDebug() << visible;

    if(d->visible != visible) {
        d->visible = visible;
        Q_EMIT visibleToUserChanged(visible);
    }

    if(visible) {
        acknowledgeAllMessages();
    }
}

bool MessagesModel::isVisibleToUser() const
{
    return d->visible;
}

MessagesModel::~MessagesModel()
{
    kDebug();
    delete d;
}

void MessagesModel::printallmessages()
{
    Q_FOREACH(MessageItem msg, d->messages) {
        kDebug() << msg.text;
    }
}

#include "moc_messages-model.cpp"