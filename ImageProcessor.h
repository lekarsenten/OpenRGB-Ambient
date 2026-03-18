//
// Created by Kamil Rojewski on 16.07.2021.
//

#ifndef OPENRGB_AMBIENT_IMAGEPROCESSOR_H
#define OPENRGB_AMBIENT_IMAGEPROCESSOR_H

#include <string>
#include <vector>
#include <array>

#include <QtGlobal>
#include <QCoreApplication>

#include <RGBController.h>

#include "SdrHorizontalRegionProcessor.h"
#include "SdrVerticalRegionProcessor.h"
#include "HdrHorizontalRegionProcessor.h"
#include "HdrVerticalRegionProcessor.h"
#include "ColorPostProcessor.h"
#include "LedUpdateEvent.h"
#include "LedRange.h"
#include "ZoneMapping.h"

class QObject;

class ImageProcessorBase
{
public:
    virtual void processSdrImage(const uchar *data, int width, int height, int stridePixels) = 0;
    virtual void processHdrImage(const uint *data, int width, int height, int stridePixels) = 0;
};

template<ColorPostProcessor CPP>
class ImageProcessor final
        : public ImageProcessorBase
{
public:
    ImageProcessor(std::string controllerLocation,
                   int totalLeds,
                   LedRange topRange,
                   LedRange bottomRange,
                   LedRange rightRange,
                   LedRange leftRange,
                   const std::vector<ZoneLedRange> &zoneMappings,
                   std::array<float, 3> colorFactors,
                   CPP colorPostProcessor,
                   bool useZoneMapping,
                   QObject *eventReceiver)
            : controllerLocation{std::move(controllerLocation)}
            , eventReceiver{eventReceiver}
            , rightRange{rightRange}
            , leftRange{leftRange}
            , bottomRange{bottomRange}
            , topRange{topRange}
            , topSdrProcessor{topRange.getLength(), colorFactors, colorPostProcessor}
            , bottomSdrProcessor{bottomRange.getLength(), colorFactors, colorPostProcessor}
            , leftSdrProcessor{leftRange.getLength(), colorFactors, colorPostProcessor}
            , rightSdrProcessor{rightRange.getLength(), colorFactors, colorPostProcessor}
            , topHdrProcessor{topRange.getLength(), colorFactors, colorPostProcessor}
            , bottomHdrProcessor{bottomRange.getLength(), colorFactors, colorPostProcessor}
            , leftHdrProcessor{leftRange.getLength(), colorFactors, colorPostProcessor}
            , rightHdrProcessor{rightRange.getLength(), colorFactors, colorPostProcessor}
            , colors(totalLeds)
            , useZoneMapping{useZoneMapping}
    {
        for (const auto &z : zoneMappings)
        {
            const auto len = z.range.getLength();
            switch (z.region)
            {
                case ScreenRegion::Top:
                    topZoneEntries.push_back({z.range, z.reversed});
                    topSdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    topHdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    break;
                case ScreenRegion::Bottom:
                    bottomZoneEntries.push_back({z.range, z.reversed});
                    bottomSdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    bottomHdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    break;
                case ScreenRegion::Left:
                    leftZoneEntries.push_back({z.range, z.reversed});
                    leftSdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    leftHdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    break;
                case ScreenRegion::Right:
                    rightZoneEntries.push_back({z.range, z.reversed});
                    rightSdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    rightHdrZoneProcs.emplace_back(len, colorFactors, colorPostProcessor);
                    break;
                default:
                    break;
            }
        }
    }

    void processSdrImage(const uchar *data, int width, int height, int stridePixels) override
    {
        const auto sampleHeight = height / resolutionHeightDivisor;
        const auto sampleWidth = width / resolutionWidthDivisor;

        const auto realTop = std::min(topRange.from, topRange.to);
        const auto realBottom = std::min(bottomRange.from, bottomRange.to);
        const auto realRight = std::min(rightRange.from, rightRange.to);
        const auto realLeft = std::min(leftRange.from, leftRange.to);

        if (!useZoneMapping)
        {
            topSdrProcessor.processRegion(colors.data() + realTop, data, width, sampleHeight, stridePixels);
            bottomSdrProcessor.processRegion(colors.data() + realBottom, data + 4 * stridePixels * (height - sampleHeight), width, sampleHeight, stridePixels);
            leftSdrProcessor.processRegion(colors.data() + realLeft, data, sampleWidth, height, 0, stridePixels);
            rightSdrProcessor.processRegion(colors.data() + realRight, data, sampleWidth, height, width - sampleWidth, stridePixels);
        }
        else
        {
            for (auto i = 0u; i < topZoneEntries.size(); ++i)
            {
                const auto start = std::min(topZoneEntries[i].range.from, topZoneEntries[i].range.to);
                const auto len = topZoneEntries[i].range.getLength();
                topSdrZoneProcs[i].processRegion(colors.data() + start, data, width, sampleHeight, stridePixels);
                if (topZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < bottomZoneEntries.size(); ++i)
            {
                const auto start = std::min(bottomZoneEntries[i].range.from, bottomZoneEntries[i].range.to);
                const auto len = bottomZoneEntries[i].range.getLength();
                bottomSdrZoneProcs[i].processRegion(colors.data() + start, data + 4 * stridePixels * (height - sampleHeight), width, sampleHeight, stridePixels);
                if (bottomZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < leftZoneEntries.size(); ++i)
            {
                const auto start = std::min(leftZoneEntries[i].range.from, leftZoneEntries[i].range.to);
                const auto len = leftZoneEntries[i].range.getLength();
                leftSdrZoneProcs[i].processRegion(colors.data() + start, data, sampleWidth, height, 0, stridePixels);
                if (leftZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < rightZoneEntries.size(); ++i)
            {
                const auto start = std::min(rightZoneEntries[i].range.from, rightZoneEntries[i].range.to);
                const auto len = rightZoneEntries[i].range.getLength();
                rightSdrZoneProcs[i].processRegion(colors.data() + start, data, sampleWidth, height, width - sampleWidth, stridePixels);
                if (rightZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
        }

        QCoreApplication::postEvent(eventReceiver, new LedUpdateEvent{controllerLocation, colors});
    }

    void processHdrImage(const uint *data, int width, int height, int stridePixels) override
    {
        const auto sampleHeight = height / resolutionHeightDivisor;
        const auto sampleWidth = width / resolutionWidthDivisor;

        const auto realTop = std::min(topRange.from, topRange.to);
        const auto realBottom = std::min(bottomRange.from, bottomRange.to);
        const auto realRight = std::min(rightRange.from, rightRange.to);
        const auto realLeft = std::min(leftRange.from, leftRange.to);

        if (!useZoneMapping)
        {
            topHdrProcessor.processRegion(colors.data() + realTop, data, width, sampleHeight, stridePixels);
            bottomHdrProcessor.processRegion(colors.data() + realBottom, data + stridePixels * (height - sampleHeight), width, sampleHeight, stridePixels);
            leftHdrProcessor.processRegion(colors.data() + realLeft, data, sampleWidth, height, 0, stridePixels);
            rightHdrProcessor.processRegion(colors.data() + realRight, data, sampleWidth, height, width - sampleWidth, stridePixels);
        }
        else
        {
            for (auto i = 0u; i < topZoneEntries.size(); ++i)
            {
                const auto start = std::min(topZoneEntries[i].range.from, topZoneEntries[i].range.to);
                const auto len = topZoneEntries[i].range.getLength();
                topHdrZoneProcs[i].processRegion(colors.data() + start, data, width, sampleHeight, stridePixels);
                if (topZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < bottomZoneEntries.size(); ++i)
            {
                const auto start = std::min(bottomZoneEntries[i].range.from, bottomZoneEntries[i].range.to);
                const auto len = bottomZoneEntries[i].range.getLength();
                bottomHdrZoneProcs[i].processRegion(colors.data() + start, data + stridePixels * (height - sampleHeight), width, sampleHeight, stridePixels);
                if (bottomZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < leftZoneEntries.size(); ++i)
            {
                const auto start = std::min(leftZoneEntries[i].range.from, leftZoneEntries[i].range.to);
                const auto len = leftZoneEntries[i].range.getLength();
                leftHdrZoneProcs[i].processRegion(colors.data() + start, data, sampleWidth, height, 0, stridePixels);
                if (leftZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
            for (auto i = 0u; i < rightZoneEntries.size(); ++i)
            {
                const auto start = std::min(rightZoneEntries[i].range.from, rightZoneEntries[i].range.to);
                const auto len = rightZoneEntries[i].range.getLength();
                rightHdrZoneProcs[i].processRegion(colors.data() + start, data, sampleWidth, height, width - sampleWidth, stridePixels);
                if (rightZoneEntries[i].reversed)
                    std::reverse(colors.data() + start, colors.data() + start + len);
            }
        }

        QCoreApplication::postEvent(eventReceiver, new LedUpdateEvent{controllerLocation, colors});
    }

private:
    static const int resolutionHeightDivisor = 10;
    static const int resolutionWidthDivisor = 12;

    std::string controllerLocation;
    QObject *eventReceiver;
    bool useZoneMapping = false;

    LedRange topRange;
    LedRange bottomRange;
    LedRange leftRange;
    LedRange rightRange;

    SdrHorizontalRegionProcessor<CPP> topSdrProcessor;
    SdrHorizontalRegionProcessor<CPP> bottomSdrProcessor;
    SdrVerticalRegionProcessor<CPP> leftSdrProcessor;
    SdrVerticalRegionProcessor<CPP> rightSdrProcessor;

    HdrHorizontalRegionProcessor<CPP> topHdrProcessor;
    HdrHorizontalRegionProcessor<CPP> bottomHdrProcessor;
    HdrVerticalRegionProcessor<CPP> leftHdrProcessor;
    HdrVerticalRegionProcessor<CPP> rightHdrProcessor;

    struct ZoneEntry { LedRange range; bool reversed; };

    std::vector<ZoneEntry> topZoneEntries;
    std::vector<ZoneEntry> bottomZoneEntries;
    std::vector<ZoneEntry> leftZoneEntries;
    std::vector<ZoneEntry> rightZoneEntries;

    std::vector<SdrHorizontalRegionProcessor<CPP>> topSdrZoneProcs;
    std::vector<SdrHorizontalRegionProcessor<CPP>> bottomSdrZoneProcs;
    std::vector<SdrVerticalRegionProcessor<CPP>>   leftSdrZoneProcs;
    std::vector<SdrVerticalRegionProcessor<CPP>>   rightSdrZoneProcs;

    std::vector<HdrHorizontalRegionProcessor<CPP>> topHdrZoneProcs;
    std::vector<HdrHorizontalRegionProcessor<CPP>> bottomHdrZoneProcs;
    std::vector<HdrVerticalRegionProcessor<CPP>>   leftHdrZoneProcs;
    std::vector<HdrVerticalRegionProcessor<CPP>>   rightHdrZoneProcs;

    std::vector<RGBColor> colors;
};

#endif //OPENRGB_AMBIENT_IMAGEPROCESSOR_H
