//
// Created by Kamil Rojewski on 15.07.2021.
//
#include <algorithm>

#include <QHBoxLayout>
#include <QTreeWidget>

#include <ResourceManager.h>
#include <RGBController.h>

#include "Settings.h"

#include "DeviceList.h"

static constexpr int LOC_ROLE      = Qt::UserRole;
static constexpr int ZONE_NAME_ROLE = Qt::UserRole + 1;

DeviceList::DeviceList(ResourceManagerInterface *resourceManager, Settings &settings, QWidget *parent)
    : QWidget{parent}
    , resourceManager{resourceManager}
    , settings{settings}
{
    const auto layout = new QHBoxLayout{this};

    deviceList = new QTreeWidget{};
    deviceList->setHeaderHidden(true);
    deviceList->setRootIsDecorated(true);

    connect(deviceList, &QTreeWidget::itemChanged, this, &DeviceList::onItemChanged);
    connect(deviceList, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (current == nullptr)
            return;
        const auto location = current->data(LOC_ROLE, Qt::DisplayRole).toString();
        // data stored via setData(column, role, value) — use column 0
        const auto loc = current->data(0, LOC_ROLE).toString();
        if (!loc.isEmpty())
            emit controllerSelected(loc);
    });

    layout->addWidget(deviceList);
}

void DeviceList::fillControllerList() const
{
    const QSignalBlocker blocker{deviceList};
    deviceList->clear();

    const auto &controllers = resourceManager->GetRGBControllers();
    for (const auto controller : controllers)
    {
        if (std::ranges::none_of(controller->modes, [](const auto &mode) {
            return mode.name == "Direct";
        }))
            continue;

        const auto location = QString::fromStdString(controller->location);
        const bool ctrlSelected = settings.isControllerSelected(controller->location);

        auto *ctrlItem = new QTreeWidgetItem{deviceList};
        ctrlItem->setText(0, QString::fromStdString(controller->name));
        ctrlItem->setData(0, LOC_ROLE, location);
        ctrlItem->setCheckState(0, ctrlSelected ? Qt::Checked : Qt::Unchecked);

        for (const auto &zone : controller->zones)
        {
            const auto zoneName = QString::fromStdString(zone.name);

            auto *zoneItem = new QTreeWidgetItem{ctrlItem};
            zoneItem->setText(0, zoneName);
            zoneItem->setData(0, LOC_ROLE, location);
            zoneItem->setData(0, ZONE_NAME_ROLE, zoneName);
            zoneItem->setCheckState(0, settings.isZoneEnabled(controller->location, zone.name) ? Qt::Checked : Qt::Unchecked);
            zoneItem->setDisabled(!ctrlSelected);
        }

        ctrlItem->setExpanded(true);
    }
}

void DeviceList::onItemChanged(QTreeWidgetItem *item, int) const
{
    const QSignalBlocker blocker{deviceList};

    const auto location = item->data(0, LOC_ROLE).toString().toStdString();
    const bool checked  = item->checkState(0) == Qt::Checked;

    if (item->parent() == nullptr)
    {
        // Controller item
        if (checked)
            settings.selectController(location);
        else
            settings.unselectController(location);

        for (int i = 0; i < item->childCount(); ++i)
            item->child(i)->setDisabled(!checked);
    }
    else
    {
        // Zone item
        const auto zoneName = item->data(0, ZONE_NAME_ROLE).toString().toStdString();
        settings.setZoneEnabled(location, zoneName, checked);
    }
}
