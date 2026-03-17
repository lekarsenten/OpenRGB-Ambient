//
// Created by Kamil Rojewski on 15.07.2021.
//

#include <algorithm>

#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPushButton>

#include <ResourceManager.h>

#include "RegionWidget.h"
#include "Settings.h"
#include "ZoneMapping.h"

#include "RegionsWidget.h"

#include <RGBController.h>

RegionsWidget::RegionsWidget(ResourceManagerInterface *resourceManager, Settings &settings, QWidget *parent)
    : QWidget{parent}
    , resourceManager{resourceManager}
    , settings{settings}
{
    const auto layout = new QFormLayout{this};

    top = new RegionWidget{};
    connect(top, &RegionWidget::regionChanged, this, [=](auto from, auto to) {
        if (!currentLocation.empty())
        {
            this->settings.setTopRegion(currentLocation, {from, to});
            if (!preview)
                showCurrentLeds(from, to);
        }
    });
    connect(top, &RegionWidget::adjustmentFinished, this, [=]() {
        if (!preview)
            clearCurrentLeds();
    });

    layout->addRow("Top", top);

    bottom = new RegionWidget{};
    connect(bottom, &RegionWidget::regionChanged, this, [=](auto from, auto to) {
        if (!currentLocation.empty())
        {
            this->settings.setBottomRegion(currentLocation, {from, to});
            if (!preview)
                showCurrentLeds(from, to);
        }
    });
    connect(bottom, &RegionWidget::adjustmentFinished, this, [=]() {
        if (!preview)
            clearCurrentLeds();
    });

    layout->addRow("Bottom", bottom);

    right = new RegionWidget{};
    connect(right, &RegionWidget::regionChanged, this, [=](auto from, auto to) {
        if (!currentLocation.empty())
        {
            this->settings.setRightRegion(currentLocation, {from, to});
            if (!preview)
                showCurrentLeds(from, to);
        }
    });
    connect(right, &RegionWidget::adjustmentFinished, this, [=]() {
        if (!preview)
            clearCurrentLeds();
    });

    layout->addRow("Right", right);

    left = new RegionWidget{};
    connect(left, &RegionWidget::regionChanged, this, [=](auto from, auto to) {
        if (!currentLocation.empty())
        {
            this->settings.setLeftRegion(currentLocation, {from, to});
            if (!preview)
                showCurrentLeds(from, to);
        }
    });
    connect(left, &RegionWidget::adjustmentFinished, this, [=]() {
        if (!preview)
            clearCurrentLeds();
    });

    layout->addRow("Left", left);

    layout->addRow(new QLabel{"<b>Zone mapping</b>"});

    zonesContainer = new QWidget{};
    zonesLayout = new QFormLayout{zonesContainer};
    zonesLayout->setContentsMargins(0, 0, 0, 0);
    layout->addRow(zonesContainer);
}

void RegionsWidget::selectController(const QString &location)
{
    currentLocation = location.toStdString();

    const auto &controllers = resourceManager->GetRGBControllers();
    const auto controller = std::find_if(std::begin(controllers), std::end(controllers), [&](auto controller) {
        return controller->location == currentLocation;
    });

    if (controller == std::end(controllers))
    {
        top->setConfiguration(0, 0, 0);
        bottom->setConfiguration(0, 0, 0);
        right->setConfiguration(0, 0, 0);
        left->setConfiguration(0, 0, 0);

        rebuildZoneRows();
        return;
    }

    const auto topRange = settings.getTopRegion(currentLocation);
    const auto bottomRange = settings.getBottomRegion(currentLocation);
    const auto rightRange = settings.getRightRegion(currentLocation);
    const auto leftRange = settings.getLeftRegion(currentLocation);

    const auto max = static_cast<int>((*controller)->leds.size()) + 1;

    top->setConfiguration(max, topRange.from, topRange.to);
    bottom->setConfiguration(max, bottomRange.from, bottomRange.to);
    right->setConfiguration(max, rightRange.from, rightRange.to);
    left->setConfiguration(max, leftRange.from, leftRange.to);

    rebuildZoneRows();
}

void RegionsWidget::showCurrentLeds(int from, int to)
{
    const auto &controllers = resourceManager->GetRGBControllers();
    const auto controller = std::ranges::find_if(controllers, [&](auto controller) {
        return controller->location == currentLocation;
    });

    if (controller == std::end(controllers))
        return;

    const auto realFrom = std::min(from, to);
    const auto realTo = std::max(from, to);

    const auto len = (*controller)->leds.size();

    for (auto i = 0; i < realFrom; ++i)
        (*controller)->SetLED(i, 0);

    for (auto i = realFrom; i < realTo; ++i)
        (*controller)->SetLED(i, ToRGBColor(255, 255, 255));

    for (auto i = realTo; i < len; ++i)
        (*controller)->SetLED(i, 0);

    (*controller)->UpdateLEDs();
}

void RegionsWidget::clearCurrentLeds()
{
    const auto &controllers = resourceManager->GetRGBControllers();
    const auto controller = std::ranges::find_if(controllers, [&](auto controller) {
        return controller->location == currentLocation;
    });

    if (controller == std::end(controllers))
        return;

    const auto len = (*controller)->leds.size();
    for (auto i = 0; i < len; ++i)
        (*controller)->SetLED(i, 0);

    (*controller)->UpdateLEDs();
}

void RegionsWidget::setPreview(bool enabled)
{
    preview = enabled;
}

void RegionsWidget::rebuildZoneRows()
{
    while (zonesLayout->rowCount() > 0)
        zonesLayout->removeRow(0);

    if (currentLocation.empty())
        return;

    const auto &controllers = resourceManager->GetRGBControllers();
    const auto controllerIt = std::ranges::find_if(controllers, [&](auto c) {
        return c->location == currentLocation;
    });

    if (controllerIt == std::end(controllers))
        return;

    static const QStringList regionNames = {"None", "Top", "Bottom", "Left", "Right"};

    const auto loc = currentLocation;

    for (const auto &zone : (*controllerIt)->zones)
    {
        const auto zoneName  = zone.name;
        const auto maxLeds   = static_cast<int>(zone.leds_count);
        const auto segsCopy  = zone.segments;

        // --- Zone header row ---
        const auto headerWidget = new QWidget{};
        const auto headerLayout = new QHBoxLayout{headerWidget};
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->addWidget(new QLabel{QString("<b>%1</b>").arg(QString::fromStdString(zoneName))});

        if (!segsCopy.empty())
        {
            const auto loadBtn = new QPushButton{"Load segments"};
            connect(loadBtn, &QPushButton::clicked, this, [=]() {
                std::vector<ZonePart> parts;
                for (const auto &seg : segsCopy)
                    parts.push_back({static_cast<int>(seg.start_idx),
                                     static_cast<int>(seg.start_idx + seg.leds_count),
                                     ScreenRegion::None, false});
                this->settings.setZoneParts(loc, zoneName, parts);
                QMetaObject::invokeMethod(this, &RegionsWidget::rebuildZoneRows, Qt::QueuedConnection);
            });
            headerLayout->addWidget(loadBtn);
        }

        const auto addBtn = new QPushButton{"+"};
        addBtn->setMaximumWidth(28);
        connect(addBtn, &QPushButton::clicked, this, [=]() {
            auto parts = this->settings.getZoneParts(loc, zoneName);
            parts.push_back({0, maxLeds, ScreenRegion::None, false});
            this->settings.setZoneParts(loc, zoneName, parts);
            QMetaObject::invokeMethod(this, &RegionsWidget::rebuildZoneRows, Qt::QueuedConnection);
        });
        headerLayout->addWidget(addBtn);
        headerLayout->addStretch();

        zonesLayout->addRow(headerWidget);

        // --- One row per part ---
        const auto parts = settings.getZoneParts(loc, zoneName);
        for (int partIdx = 0; partIdx < static_cast<int>(parts.size()); ++partIdx)
        {
            const auto &part = parts[partIdx];

            const auto fromSpin = new QSpinBox{};
            fromSpin->setRange(0, maxLeds);
            fromSpin->setValue(part.from);
            fromSpin->setPrefix("from ");

            const auto toSpin = new QSpinBox{};
            toSpin->setRange(0, maxLeds);
            toSpin->setValue(part.to);
            toSpin->setPrefix("to ");

            const auto regionCombo = new QComboBox{};
            for (int i = 0; i < regionNames.size(); ++i)
                regionCombo->addItem(regionNames[i], i);
            regionCombo->setCurrentIndex(static_cast<int>(part.region));

            const auto reversedCheck = new QCheckBox{"Rev"};
            reversedCheck->setChecked(part.reversed);
            reversedCheck->setEnabled(part.region != ScreenRegion::None);

            const auto removeBtn = new QPushButton{"×"};
            removeBtn->setMaximumWidth(25);

            // Save current values of this part back to settings (no rebuild)
            auto savePart = [=]() {
                auto current = this->settings.getZoneParts(loc, zoneName);
                if (partIdx < static_cast<int>(current.size()))
                {
                    current[partIdx] = {
                        fromSpin->value(),
                        toSpin->value(),
                        static_cast<ScreenRegion>(regionCombo->currentData().toInt()),
                        reversedCheck->isChecked()
                    };
                    this->settings.setZoneParts(loc, zoneName, current);
                }
            };

            connect(fromSpin,     QOverload<int>::of(&QSpinBox::valueChanged),  this, savePart);
            connect(toSpin,       QOverload<int>::of(&QSpinBox::valueChanged),  this, savePart);
            connect(regionCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
                reversedCheck->setEnabled(static_cast<ScreenRegion>(idx) != ScreenRegion::None);
                savePart();
            });
            connect(reversedCheck, &QCheckBox::toggled, this, savePart);
            connect(removeBtn, &QPushButton::clicked, this, [=]() {
                auto current = this->settings.getZoneParts(loc, zoneName);
                if (partIdx < static_cast<int>(current.size()))
                    current.erase(current.begin() + partIdx);
                this->settings.setZoneParts(loc, zoneName, current);
                QMetaObject::invokeMethod(this, &RegionsWidget::rebuildZoneRows, Qt::QueuedConnection);
            });

            const auto partWidget = new QWidget{};
            const auto partLayout = new QHBoxLayout{partWidget};
            partLayout->setContentsMargins(16, 0, 0, 0);
            partLayout->addWidget(fromSpin);
            partLayout->addWidget(toSpin);
            partLayout->addWidget(regionCombo);
            partLayout->addWidget(reversedCheck);
            partLayout->addWidget(removeBtn);

            zonesLayout->addRow(partWidget);
        }
    }
}
