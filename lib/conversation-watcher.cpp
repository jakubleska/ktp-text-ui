/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

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


#include "conversation-watcher.h"

#include <KDebug>

#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/TextChannel>
#include "conversation.h"
#include <TelepathyQt4/ClientRegistrar>


static inline Tp::ChannelClassSpecList channelClassList()
{
    return Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChat()
                                      << Tp::ChannelClassSpec::unnamedTextChat()
                                      << Tp::ChannelClassSpec::textChatroom();
}

class ConversationWatcher::ConversationClientObserver :
    public Tp::AbstractClientObserver
{
public:
    virtual void observeChannels(
        const Tp::MethodInvocationContextPtr<>& context,
        const Tp::AccountPtr& account,
        const Tp::ConnectionPtr& connection,
        const QList< Tp::ChannelPtr >& channels,
        const Tp::ChannelDispatchOperationPtr& dispatchOperation,
        const QList< Tp::ChannelRequestPtr >& requestsSatisfied,
        const Tp::AbstractClientObserver::ObserverInfo& observerInfo)
    {
        kDebug();

        Tp::TextChannelPtr textChannel;
        Q_FOREACH(const Tp::ChannelPtr & channel, channels) {
            textChannel = Tp::TextChannelPtr::dynamicCast(channel);
            if (textChannel) {
                break;
            }
        }

        Q_ASSERT(textChannel);

        Conversation *con = new Conversation(textChannel, account);
        m_parent->newConversation(con);
    }

    ConversationClientObserver(ConversationWatcher *parent) :
        AbstractClientObserver(channelClassList()),
        m_parent(parent)
    {
    }

    ConversationWatcher *m_parent;
    Tp::ClientRegistrarPtr registrar;
};

ConversationWatcher::ConversationWatcher() :
    d(new ConversationClientObserver(this))
{
    kDebug();
    Tp::registerTypes();
    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(QDBusConnection::sessionBus(),
                                                                      Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr  connectionFactory = Tp::ConnectionFactory::create(
        QDBusConnection::sessionBus(),
        Tp::Features() << Tp::Connection::FeatureSelfContact
                       << Tp::Connection::FeatureCore
    );

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());
    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);

    Tp::Features textFeatures = Tp::Features() << Tp::TextChannel::FeatureMessageQueue
                                               << Tp::TextChannel::FeatureMessageSentSignal
                                               << Tp::TextChannel::FeatureChatState
                                               << Tp::TextChannel::FeatureMessageCapabilities;
    channelFactory->addFeaturesForTextChats(textFeatures);
    channelFactory->addFeaturesForTextChatrooms(textFeatures);

    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create(
        Tp::Features() << Tp::Contact::FeatureAlias
                       << Tp::Contact::FeatureAvatarToken
                       << Tp::Contact::FeatureAvatarData
                       << Tp::Contact::FeatureCapabilities
                       << Tp::Contact::FeatureSimplePresence
    );

    d->registrar = Tp::ClientRegistrar::create(accountFactory, connectionFactory,
                                               channelFactory, contactFactory);

    d->registrar->registerClient(d, QLatin1String("KDE.TextUi.ConversationWatcher"));
}


ConversationWatcher::~ConversationWatcher()
{
}

#include "moc_conversation-watcher.cpp"