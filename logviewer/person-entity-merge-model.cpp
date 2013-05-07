/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "person-entity-merge-model.h"
#include "entity-model.h"

#include <kpeople/persons-model.h>
#include <kpeople/persondata.h>

#include <TelepathyLoggerQt4/LogManager>
#include <TelepathyLoggerQt4/PendingEntities>
#include <TelepathyLoggerQt4/Entity>
#include <TelepathyLoggerQt4/SearchHit>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>

#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KIconLoader>

class PersonEntityMergeModel::Item
{
  public:
    explicit Item():
        parent(0)
    {
    }

    virtual ~Item()
    {
        qDeleteAll(children);
        children.clear();
    }

    void addChild(Item *item)
    {
        children << item;
        item->parent = this;
    }

    Item *parent;
    QList<Item*> children;
};

class PersonEntityMergeModel::GroupItem: public Item
{
  public:
    explicit GroupItem():
        Item()
    {}

    virtual ~GroupItem()
    {
    }

    QString label;
};

class PersonEntityMergeModel::ContactItem: public Item
{
  public:
    explicit ContactItem():
        Item()
    {}

    virtual ~ContactItem()
    {
    }

    bool isPersona() const
    {
        return personaIndex.isValid() && !contactIndex.isValid();
    }

    QPersistentModelIndex personaIndex;
    QPersistentModelIndex contactIndex;
    QPersistentModelIndex entityIndex;

};

PersonEntityMergeModel::ContactItem* PersonEntityMergeModel::itemForPersona(const QModelIndex& personsModel_personaIndex)
{
    if (!personsModel_personaIndex.isValid()) {
        return 0;
    }

    // FIXME: This is slow...
    Q_FOREACH (Item *group, m_rootItem->children) {
        Q_FOREACH (Item *item, group->children) {
            ContactItem *cItem = dynamic_cast<ContactItem*>(item);
            Q_ASSERT(cItem);
            if (cItem->personaIndex == personsModel_personaIndex) {
                kDebug() << "\t\tFound existing persona for" << personsModel_personaIndex.data();
                return cItem;
            }
        }
    }

    kDebug() << "\t\tCreating a new persona for" << personsModel_personaIndex.data();
    ContactItem *item = new ContactItem;
    item->personaIndex = personsModel_personaIndex;

    GroupItem *groupItem = groupForName(personsModel_personaIndex.data(PersonsModel::ContactGroupsRole));
    groupItem->addChild(item);

    return item;
}

PersonEntityMergeModel::GroupItem* PersonEntityMergeModel::groupForName(const QVariant& data)
{
    QString groupName;

    if (data.canConvert<QVariantList>()) {
        const QVariantList list = data.toList();
        if (!list.isEmpty()) {
            // FIXME: How is it with multi-group membership?
            groupName = list.first().toString();
        }
    } else {
        groupName = data.toString();
    }

    if (groupName.isEmpty()) {
        groupName = i18n("Unsorted");
    }

    Q_FOREACH (Item *item, m_rootItem->children) {
        GroupItem *group = dynamic_cast<GroupItem*>(item);
        Q_ASSERT(group);

        if (group->label == groupName) {
            kDebug() << "\tFound matching group" << groupName;
            return group;
        }
    }

    kDebug() << "\tCreating a new group" << groupName;
    GroupItem *group = new GroupItem;
    group->label = groupName;
    m_rootItem->addChild(group);

    return group;
}

PersonEntityMergeModel::Item* PersonEntityMergeModel::itemForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return m_rootItem;
    }

    Item *parent = static_cast<Item*>(index.internalPointer());
    Q_ASSERT(parent != 0);

    Q_ASSERT(index.row() < parent->children.count());
    Item *item = parent->children.at(index.row());
    Q_ASSERT(item != 0);

    return item;
}

PersonEntityMergeModel::PersonEntityMergeModel(PersonsModel* personsModel,
                                               EntityModel* entityModel,
                                               QObject* parent):
    QAbstractItemModel(parent),
    m_personsModel(personsModel),
    m_entityModel(entityModel),
    m_rootItem(new ContactItem)
{
    connect(m_personsModel, SIGNAL(peopleAdded()),
            this, SLOT(initializeModel()));
}

PersonEntityMergeModel::~PersonEntityMergeModel()
{
    delete m_rootItem;
}

int PersonEntityMergeModel::rowCount(const QModelIndex& parent) const
{
    Item *parentItem = itemForIndex(parent);
    return parentItem->children.count();
}

int PersonEntityMergeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return 1;
}

Qt::ItemFlags PersonEntityMergeModel::flags(const QModelIndex& index) const
{
    // FIXME: Make Persons selectable and show some fancy stuff
    /*
    if (index.data(PersonsModel::ResourceTypeRole).toUInt() == PersonsModel::Person) {
        return Qt::ItemIsEnabled;
    }
    */

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant PersonEntityMergeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    // If parent is m_rootItem, then we are dealing with a group
    if (index.internalPointer() == m_rootItem) {
        GroupItem *item = dynamic_cast<GroupItem*>(itemForIndex(index));
        Q_ASSERT(item);

        if (role == Qt::DisplayRole) {
            return item->label;
        } else if (role == PersonEntityMergeModel::ItemTypeRole) {
            return PersonEntityMergeModel::Group;
        }

        // FIXME: We could support more roles?
        return QVariant();
    }


    ContactItem *item = dynamic_cast<ContactItem*>(itemForIndex(index));
    Q_ASSERT(item);

    switch (role) {
        case PersonEntityMergeModel::EntityRole:
            return item->entityIndex.data(EntityModel::EntityRole);
        case PersonEntityMergeModel::ContactRole:
            return item->contactIndex.data(PersonsModel::IMContactRole);
        case PersonEntityMergeModel::AccountRole:
            return item->entityIndex.data(EntityModel::AccountRole);
        case PersonEntityMergeModel::ItemTypeRole:
            return item->isPersona() ? PersonEntityMergeModel::Persona
                                     : PersonEntityMergeModel::Entity;
    }

    // Extract role from respective parent model
    QVariant value;
    if (item->isPersona()) {
        value = m_personsModel->data(item->personaIndex, role);
    } else if (item->contactIndex.isValid()) {
        value = m_personsModel->data(item->contactIndex, role);
        if (value.isNull()) {
            value = m_entityModel->data(item->entityIndex, role);
        }
    } else {
        value = m_entityModel->data(item->entityIndex, role);
    }

    // Use the blue pawn avatar if none is provided
    if (role == Qt::DecorationRole && value.isNull()) {
        value = KIconLoader::global()->loadIcon(QLatin1String("im-user"), KIconLoader::NoGroup, KIconLoader::SizeSmall);
    }

    return value;
}

QModelIndex PersonEntityMergeModel::index(int row, int column, const QModelIndex& parent) const
{
    return createIndex(row, column, itemForIndex(parent));
}

QModelIndex PersonEntityMergeModel::parent(const QModelIndex& child) const
{
    Item *parent = static_cast<Item*>(child.internalPointer());
    if (!parent->parent) {
        return QModelIndex();
    }

    Item *parentParent = parent->parent;
    return createIndex(parentParent->children.indexOf(parent), 0, parentParent);
}

void PersonEntityMergeModel::initializeModel()
{
    beginResetModel();
    for (int i = 0; i < m_entityModel->rowCount(QModelIndex()); ++i) {
        const QModelIndex entityIndex = m_entityModel->index(i, 0);
        QModelIndex contactIndex;
        QModelIndex personaIndex;
        Item *parentItem = 0;

        const Tpl::EntityPtr entity = entityIndex.data(EntityModel::EntityRole).value<Tpl::EntityPtr>();
        const Tp::AccountPtr account = entityIndex.data(EntityModel::AccountRole).value<Tp::AccountPtr>();
        kDebug() << "Searching for match for entity" << entity->identifier() << "@" << account->uniqueIdentifier();
        for (int j = 0; j < m_personsModel->rowCount(); ++j) {
            const QModelIndex index = m_personsModel->index(j, 0);
            bool found = false;

            // Is it a contact...
            if (m_personsModel->data(index, PersonsModel::ResourceTypeRole).toUInt() == PersonsModel::Contact) {
                if (m_personsModel->data(index, PersonsModel::IMRole).toString() == entity->identifier()) {
                    kDebug() << "\tFound matching contact" << m_personsModel->data(index, PersonsModel::IMRole).toString();
                    parentItem = groupForName(m_personsModel->data(index, PersonsModel::ContactGroupsRole));
                    contactIndex = index;
                    break;
                }

            // ...or a persona?
            } else {
                for (int k = 0; k < m_personsModel->rowCount(index); ++k) {
                    const QModelIndex childIndex = m_personsModel->index(k, 0, index);
                    if (m_personsModel->data(childIndex, PersonsModel::IMRole).toString() == entity->identifier()) {
                        kDebug() << "\tFound matching persona" << m_personsModel->data(index, PersonsModel::UriRole).toString();
                        kDebug() << "\t\tFound matching contact" << m_personsModel->data(childIndex, PersonsModel::IMRole).toString();
                        parentItem = itemForPersona(index);
                        personaIndex = index;
                        contactIndex = childIndex;
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                break;
            }
        }

        if (!contactIndex.isValid() && !personaIndex.isValid()) {
            kDebug() << "\tNo match";
            parentItem = groupForName(QString());
        }

        ContactItem *item = new ContactItem;
        item->entityIndex = entityIndex;
        item->contactIndex = contactIndex;
        item->personaIndex = personaIndex;
        parentItem->addChild(item);
    }
    endResetModel();
}



/*
bool PersonEntityMergeModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (!sourceModel()) {
        return false;
    }

    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!sourceIndex.isValid()) {
        return false;
    }

    // Accept Person only when it has at least one contact with logs
    if (sourceIndex.data(PersonsModel::ResourceTypeRole).toUInt() == PersonsModel::Person) {
        for (int i = 0; i < sourceModel()->rowCount(sourceParent); ++i) {
            if (filterAcceptsRow(i, sourceIndex)) {
                return true;
            }
        }
        return false;
    }

    const Tp::AccountPtr account = sourceIndex.data(PersonsModel::IMAccountRole).value<Tp::AccountPtr>();
    // FIXME: Find out why PersonsModel::IMContactRole returns NULL ptr all the time
    const QString contactUid  = sourceIndex.data(PersonsModel::IMRole).toString();
    const Tpl::EntityPtr entity = findEntity(account, contactUid);
    if (entity.isNull()) {
        return false;
    }

    if (m_searchHits.isEmpty()) {
        return true;
    } else {
        Q_FOREACH( Tpl::SearchHit searchHit, m_searchHits) {
            if ((searchHit.account() == account) &&
                (searchHit.target() == entity))
            {
                return true;
            }
        }
    }

    return false;
}
*/

#include "person-entity-merge-model.moc"