/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2023 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "inputresourceitem.h"

#include <optional>

#include <QList>

#include "log.h"
#include "stringutils.h"
#include "translation.h"
#include "types/string.h"

#include "audio/common/soundfonttypes.h"
#include "audio/common/audioutils.h"

#include "msbasicpresetscategories.h"

using namespace mu;
using namespace mu::playback;
using namespace muse;
using namespace muse::audio;
using namespace muse::audio::synth;

static const QString VST_MENU_ITEM_ID("VST3");
static const QString SOUNDFONTS_MENU_ITEM_ID("SoundFonts");
static const QString MUSE_MENU_ITEM_ID("MuseSounds");
static const QString GET_MORE_SOUNDS_ID("getMoreSounds");

static const muse::String MS_BASIC_SOUNDFONT_NAME(u"MS Basic");

static std::unordered_map<AudioResourceType, QString> AUDIO_RESOURCE_TYPE_TO_STR {
    { AudioResourceType::FluidSoundfont, SOUNDFONTS_MENU_ITEM_ID },
    { AudioResourceType::VstPlugin, VST_MENU_ITEM_ID },
    { AudioResourceType::MuseSamplerSoundPack, MUSE_MENU_ITEM_ID },
};

static QString makeMenuResourceItemId(AudioResourceType type, const QString& resourceId)
{
    QString str = muse::value(AUDIO_RESOURCE_TYPE_TO_STR, type);
    IF_ASSERT_FAILED(!str.isEmpty()) {
        return QString();
    }

    return str + "\\" + resourceId;
}

static void parseAudioResourceTypeAndId(const QString& menuItemId, AudioResourceType& type, AudioResourceId& resourceId)
{
    QString typeStr = menuItemId.section("\\", 0, 0);
    type = muse::key(AUDIO_RESOURCE_TYPE_TO_STR, typeStr);
    resourceId = menuItemId.section("\\", 1).toStdString();
}

InputResourceItem::InputResourceItem(QObject* parent)
    : AbstractAudioResourceItem(parent)
{
}

void InputResourceItem::requestAvailableResources()
{
    playback()->availableInputResources()
    .onResolve(this, [this](const AudioResourceMetaList& availableResources) {
        updateAvailableResources(availableResources);
        emit resourceListChanged(buildResourceList());
    })
    .onReject(this, [](const int errCode, const std::string& errText) {
        LOGE() << "Unable to resolve available output resources"
               << " , errCode:" << errCode
               << " , errText:" << errText;
    });
}

void InputResourceItem::handleMenuItem(const QString& menuItemId)
{
    if (menuItemId == GET_MORE_SOUNDS_ID) {
        const QString url = QString::fromStdString(globalConfiguration()->museHubWebUrl());
        const QString urlParams("muse-sounds?utm_source=mss-mixer&utm_medium=mh&utm_campaign=mss-mixer-ms-mainpage");
        interactive()->openUrl(url + urlParams);
        return;
    }

    AudioResourceType newResourceType = AudioResourceType::Undefined;
    AudioResourceId newResourceId;
    parseAudioResourceTypeAndId(menuItemId, newResourceType, newResourceId);

    auto resourcesIt = m_availableResourceMap.find(newResourceType);
    if (resourcesIt == m_availableResourceMap.cend()) {
        LOGE() << "Resource not found: " << menuItemId;
        return;
    }

    for (const auto& pairByVendor : resourcesIt->second) {
        for (const AudioResourceMeta& resourceMeta : pairByVendor.second) {
            if (newResourceId != resourceMeta.id) {
                continue;
            }

            if (m_currentInputParams.resourceMeta == resourceMeta) {
                continue;
            }

            emit inputParamsChangeRequested(resourceMeta);
            return;
        }
    }
}

const AudioInputParams& InputResourceItem::params() const
{
    return m_currentInputParams;
}

void InputResourceItem::setParams(const audio::AudioInputParams& newParams)
{
    m_currentInputParams = newParams;

    emit titleChanged();
    emit isBlankChanged();
    emit isActiveChanged();
}

void InputResourceItem::setParamsRecourceMeta(const AudioResourceMeta& newMeta)
{
    requestToCloseNativeEditorView();

    m_currentInputParams.resourceMeta = newMeta;

    emit titleChanged();
    emit isBlankChanged();
    emit isActiveChanged();
    emit inputParamsChanged();

    requestToLaunchNativeEditorView();
}

QString InputResourceItem::title() const
{
    return audio::audioSourceName(m_currentInputParams).toQString();
}

bool InputResourceItem::isBlank() const
{
    return !m_currentInputParams.isValid();
}

bool InputResourceItem::isActive() const
{
    return m_currentInputParams.isValid();
}

bool InputResourceItem::hasNativeEditorSupport() const
{
    return m_currentInputParams.resourceMeta.hasNativeEditorSupport;
}

QVariantList InputResourceItem::buildResourceList(const QString& filterText) const
{
    return filterText.isEmpty() ? buildFlyoutMenu() : buildFilteredList(filterText);
}

QVariantList InputResourceItem::buildFlyoutMenu() const
{
    QVariantList result;
    if (!isBlank()) {
        QString currentResourceId = QString::fromStdString(m_currentInputParams.resourceMeta.id);
        result << buildMenuItem(makeMenuResourceItemId(m_currentInputParams.resourceMeta.type, currentResourceId),
                                title(), true /*checked*/);
    }

    auto museResourcesSearch = m_availableResourceMap.find(AudioResourceType::MuseSamplerSoundPack);
    if (museResourcesSearch != m_availableResourceMap.end()) {
        result << buildSeparator();
        result << buildMuseMenuItem(museResourcesSearch->second);
    }

    auto vstResourcesSearch = m_availableResourceMap.find(AudioResourceType::VstPlugin);
    if (vstResourcesSearch != m_availableResourceMap.end()) {
        result << buildSeparator();
        result << buildVstMenuItem(vstResourcesSearch->second);
    }

    auto sfResourcesSearch = m_availableResourceMap.find(AudioResourceType::FluidSoundfont);
    if (sfResourcesSearch != m_availableResourceMap.end()) {
        result << buildSeparator();
        result << buildSoundFontsMenuItem(sfResourcesSearch->second);
    }

    result << buildSeparator();
    result << buildExternalLinkMenuItem(GET_MORE_SOUNDS_ID, muse::qtrc("playback", "Get more sounds"));

    return result;
}

QVariantList InputResourceItem::buildFilteredList(const QString& filterText) const
{
    QVariantList result;
    IF_ASSERT_FAILED(!filterText.isEmpty()) {
        return result;
    }

    auto museResourcesSearch = m_availableResourceMap.find(AudioResourceType::MuseSamplerSoundPack);
    if (museResourcesSearch != m_availableResourceMap.end()) {
        const QVariantList& museList = buildMuseFilteredList(museResourcesSearch->second, filterText);
        if (!museList.empty() && !result.empty()) {
            result << buildSeparator();
        }
        result << museList;
    }

    auto vstResourcesSearch = m_availableResourceMap.find(AudioResourceType::VstPlugin);
    if (vstResourcesSearch != m_availableResourceMap.end()) {
        const QVariantList& vstList = buildVstFilteredList(vstResourcesSearch->second, filterText);
        if (!vstList.empty() && !result.empty()) {
            result << buildSeparator();
        }
        result << vstList;
    }

    auto sfResourcesSearch = m_availableResourceMap.find(AudioResourceType::FluidSoundfont);
    if (sfResourcesSearch != m_availableResourceMap.end()) {
        const QVariantList& sfList = buildSoundFontsFilteredList(sfResourcesSearch->second, filterText);
        if (!sfList.empty() && !result.empty()) {
            result << buildSeparator();
        }
        result << sfList;
    }

    if (result.empty()) {
        result << buildExternalLinkMenuItem(GET_MORE_SOUNDS_ID, muse::qtrc("playback", "Get more sounds"));
    }

    return result;
}

InputResourceItem::VendorMap InputResourceItem::getMuseVendorMap(const muse::audio::AudioResourceMetaList& metaList) const
{
    VendorMap vendorMap;

    for (const AudioResourceMeta& resourceMeta : metaList) {
        const muse::String& vendorName = resourceMeta.attributeVal(u"museVendorName");
        const muse::String& pack = resourceMeta.attributeVal(u"musePack");
        const muse::String& category = resourceMeta.attributeVal(u"museCategory");
        const muse::String& name = resourceMeta.attributeVal(u"museName");

        vendorMap[vendorName][pack][category].emplace_back(std::make_pair(resourceMeta.id, name));
    }

    return vendorMap;
}

QVariantMap InputResourceItem::buildMuseMenuItem(const ResourceByVendorMap& resourcesByVendor) const
{
    QVariantList subItemsByType;

    // Vendor -> Pack -> Category -> Instruments
    for (const auto& [_, metaList] : resourcesByVendor) {
        VendorMap vendorMap = getMuseVendorMap(metaList);

        for (const auto& [vendorName, packs] : vendorMap) {
            QVariantList subItemsByVendor;
            bool isCurrentVendor = false;

            for (const auto& [packName, categories] : packs) {
                QVariantList subItemsByPack;
                bool isCurrentPack = false;

                for (const auto& [categoryName, instruments] : categories) {
                    QVariantList subItemsByCategory;
                    bool isCurrentCategory = false;

                    for (const auto& [instId, instName] : instruments) {
                        const bool isCurrentInstrument = m_currentInputParams.resourceMeta.id == instId;
                        const QString qInstId = QString::fromStdString(instId);
                        const QString itemId = makeMenuResourceItemId(AudioResourceType::MuseSamplerSoundPack, qInstId);

                        subItemsByCategory << buildMenuItem(itemId, instName.toQString(), isCurrentInstrument);
                        isCurrentCategory = isCurrentCategory || isCurrentInstrument;
                    }

                    // Create submenu only if there are 2 or more categories
                    if (categories.size() > 1 && !categoryName.empty()) {
                        QString categoryString = categoryName.toQString();
                        subItemsByPack << buildMenuItem(categoryString,
                                                        categoryString,
                                                        isCurrentCategory,
                                                        subItemsByCategory);
                    } else {
                        subItemsByPack << subItemsByCategory;
                    }

                    isCurrentPack = isCurrentPack || isCurrentCategory;
                }

                // Create submenu only if there are 2 or more packs
                if (packs.size() > 1 && !packName.empty()) {
                    QString packString = packName.toQString();
                    subItemsByVendor << buildMenuItem(packString,
                                                      packString,
                                                      isCurrentPack,
                                                      subItemsByPack);
                } else {
                    subItemsByVendor << subItemsByPack;
                }

                isCurrentVendor = isCurrentVendor || isCurrentPack;
            }

            if (!vendorName.empty()) {
                QString vendorString = vendorName.toQString();
                subItemsByType << buildMenuItem(vendorString,
                                                vendorString,
                                                isCurrentVendor,
                                                subItemsByVendor);
            } else {
                subItemsByType << subItemsByVendor;
            }
        }
    }

    return buildMenuItem(MUSE_MENU_ITEM_ID,
                         MUSE_MENU_ITEM_ID,
                         m_currentInputParams.resourceMeta.type == AudioResourceType::MuseSamplerSoundPack,
                         subItemsByType);
}

QVariantList InputResourceItem::buildMuseFilteredList(const ResourceByVendorMap& resourcesByVendor, const QString& filterText) const
{
    QVariantList result;

    // Vendor -> Pack -> Category -> Instruments
    for (const auto& [_, metaList] : resourcesByVendor) {
        VendorMap vendorMap = getMuseVendorMap(metaList);

        for (const auto& [vendorName, packs] : vendorMap) {
            for (const auto& [packName, categories] : packs) {
                QVariantList subItems;

                QString titlePrefix;
                if (!packName.empty()) {
                    titlePrefix = packName + " - ";
                } else if (!vendorName.empty()) {
                    titlePrefix = vendorName + " - ";
                }

                for (const auto& [categoryName, instruments] : categories) {
                    for (const auto& [instId, instName] : instruments) {
                        const bool resourceChecked = m_currentInputParams.resourceMeta.id == instId;
                        const QString qInstId = QString::fromStdString(instId);
                        const QString itemId = makeMenuResourceItemId(AudioResourceType::MuseSamplerSoundPack, qInstId);
                        const QString resourceTitle = titlePrefix + instName.toQString();

                        if (resourceTitle.contains(filterText, Qt::CaseInsensitive)) {
                            subItems << buildMenuItem(itemId, resourceTitle, resourceChecked);
                        }
                    }
                }

                if (!subItems.isEmpty() && !result.isEmpty()) {
                    result << buildSeparator();
                }
                result << subItems;
            }
        }
    }

    return result;
}

QVariantMap InputResourceItem::buildVstMenuItem(const ResourceByVendorMap& resourcesByVendor) const
{
    QVariantList subItemsByType;

    for (const auto& pair : resourcesByVendor) {
        QVariantList subItemsByVendor;

        for (const AudioResourceMeta& resourceMeta : pair.second) {
            const QString resourceId = QString::fromStdString(resourceMeta.id);
            const bool resourceChecked = m_currentInputParams.resourceMeta.id == resourceMeta.id;
            subItemsByVendor << buildMenuItem(makeMenuResourceItemId(resourceMeta.type, resourceId),
                                              resourceId, resourceChecked);
        }
        const QString vendorId = QString::fromStdString(pair.first);
        const bool vendorChecked = m_currentInputParams.resourceMeta.vendor == pair.first;
        subItemsByType << buildMenuItem(vendorId, vendorId, vendorChecked, subItemsByVendor);
    }

    return buildMenuItem(VST_MENU_ITEM_ID,
                         VST_MENU_ITEM_ID,
                         m_currentInputParams.resourceMeta.type == AudioResourceType::VstPlugin,
                         subItemsByType);
}

QVariantList InputResourceItem::buildVstFilteredList(const ResourceByVendorMap& resourcesByVendor, const QString& filterText) const
{
    QVariantList result;

    for (const auto& pair : resourcesByVendor) {
        QVariantList subItems;

        const QString vendorId = QString::fromStdString(pair.first);
        for (const AudioResourceMeta& resourceMeta : pair.second) {
            const QString resourceId = QString::fromStdString(resourceMeta.id);
            const QString& resourceTitle = vendorId + " - " + resourceId;

            if (resourceTitle.contains(filterText, Qt::CaseInsensitive)) {
                const bool resourceChecked = m_currentInputParams.resourceMeta.id == resourceMeta.id;
                subItems << buildMenuItem(makeMenuResourceItemId(resourceMeta.type, resourceId),
                                          resourceTitle, resourceChecked);
            }
        }
        if (!subItems.isEmpty() && !result.empty()) {
            result << buildSeparator();
        }
        result << subItems;
    }

    return result;
}

std::optional<muse::midi::Program> InputResourceItem::currentSoundFontProgram() const
{
    std::optional<midi::Program> currentPreset = std::nullopt;

    const muse::String& currentSoundFontName = m_currentInputParams.resourceMeta.attributeVal(SOUNDFONT_NAME_ATTRIBUTE);
    if (currentSoundFontName.empty()) {
        return currentPreset;
    }

    bool bankOk = false, programOk = false;
    int currentPresetBank = m_currentInputParams.resourceMeta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
    int currentPresetProgram = m_currentInputParams.resourceMeta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);

    if (bankOk && programOk) {
        currentPreset = midi::Program(currentPresetBank, currentPresetProgram);
    }

    return currentPreset;
}

bool InputResourceItem::soundFontIsCurrent(const String& soundFont) const
{
    const muse::String currentSoundFontName = m_currentInputParams.resourceMeta.attributeVal(SOUNDFONT_NAME_ATTRIBUTE);
    const muse::String currentSoundFontId = muse::String::fromStdString(m_currentInputParams.resourceMeta.id);

    // currentSoundFontId will be equal to soundFont in the case of "choose automatically" for older files (this is a temporary fix)
    // See: https://github.com/musescore/MuseScore/pull/20316#issuecomment-1841326774
    return currentSoundFontName == soundFont || currentSoundFontId == soundFont;
}

InputResourceItem::SoundFontMap InputResourceItem::soundFontMapForResources(const ResourceByVendorMap& resourcesByVendor) const // TODO: not a great name
{
    // Group resources by SoundFont name
    SoundFontMap resourcesBySoundFont;

    for (const auto& pair : resourcesByVendor) {
        for (const AudioResourceMeta& resourceMeta : pair.second) {
            const muse::String& soundFontName = resourceMeta.attributeVal(SOUNDFONT_NAME_ATTRIBUTE);
            resourcesBySoundFont[soundFontName].push_back(resourceMeta);
        }
    }

    return resourcesBySoundFont;
}

QVariantMap InputResourceItem::buildSoundFontsMenuItem(const ResourceByVendorMap& resourcesByVendor) const
{
    const SoundFontMap soundFontMap = soundFontMapForResources(resourcesByVendor);
    IF_ASSERT_FAILED(!soundFontMap.empty()) {
        return QVariantMap();
    }

    // Sort SoundFonts by name and add them to the menu
    std::vector<muse::String> soundFonts;
    soundFonts.reserve(soundFontMap.size());
    for (const auto& pair : soundFontMap) {
        soundFonts.push_back(pair.first);
    }

    std::sort(soundFonts.begin(), soundFonts.end(), [](const muse::String& s1, const muse::String& s2) {
        return muse::strings::lessThanCaseInsensitive(s1, s2);
    });

    QVariantList soundFontItems;
    for (const muse::String& soundFont : soundFonts) {
        if (soundFont == MS_BASIC_SOUNDFONT_NAME) {
            soundFontItems << buildMsBasicMenuItem(soundFontMap.at(soundFont));
        } else {
            soundFontItems << buildOtherSoundFontMenuItem(soundFont, soundFontMap.at(soundFont));
        }
    }

    return buildMenuItem(SOUNDFONTS_MENU_ITEM_ID,
                         muse::qtrc("playback", "SoundFonts"),
                         m_currentInputParams.resourceMeta.type == AudioResourceType::FluidSoundfont,
                         soundFontItems);
}

QVariantMap InputResourceItem::buildMsBasicMenuItem(const AudioResourceMetaList& availableResources) const
{
    std::map<midi::Program, AudioResourceMeta> resourcesByProgram;
    AudioResourceMeta chooseAutomaticMeta;

    for (const AudioResourceMeta& resourceMeta : availableResources) {
        bool bankOk = false, programOk = false;
        int presetBank = resourceMeta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
        int presetProgram = resourceMeta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);

        if (bankOk && programOk) {
            resourcesByProgram[midi::Program(presetBank, presetProgram)] = resourceMeta;
        } else {
            chooseAutomaticMeta = resourceMeta;
        }
    }

    const bool isCurrentSoundFont = soundFontIsCurrent(MS_BASIC_SOUNDFONT_NAME);
    const std::optional<midi::Program>& currentPreset = currentSoundFontProgram();

    //! NOTE: This is called recursively for sub items...
    std::function<QVariantMap(const MsBasicItem&, const QString&, bool&, bool&)> doBuildMsBasicItem
        = [&](const MsBasicItem& item, const QString& parentMenuId, bool& ok, bool& isCurrent) {
        ok = true;
        if (item.subItems.empty()) { // Base case - no more subitems to scan...
            auto it = resourcesByProgram.find(item.preset);
            if (it == resourcesByProgram.cend()) {
                LOGW() << "Preset specified in MS_BASIC_PRESET_CATEGORIES not found in SoundFont: bank " << item.preset.bank
                       << ", program " << item.preset.program;

                ok = false;
                return QVariantMap();
            }

            const AudioResourceMeta& resourceMeta = it->second;

            isCurrent = isCurrentSoundFont && currentPreset.has_value() && currentPreset.value() == item.preset;

            QString presetName = resourceMeta.attributeVal(PRESET_NAME_ATTRIBUTE);
            if (presetName.isEmpty()) {
                presetName = muse::qtrc("playback", "Bank %1, preset %2").arg(item.preset.bank).arg(item.preset.program);
            }

            // Temporary fix, see: https://github.com/musescore/MuseScore/issues/20142
            if (presetName.contains(muse::String(u"Expr."))) {
                ok = false;
                return QVariantMap();
            }

            return buildMenuItem(makeMenuResourceItemId(resourceMeta.type, QString::fromStdString(resourceMeta.id)),
                                 presetName,
                                 isCurrent);
        }

        QString menuId = parentMenuId + "\\" + item.title + "\\menu";

        QVariantList subItems;

        for (const MsBasicItem& subItem : item.subItems) {
            bool _ok = false;
            bool isSubItemCurrent = false;

            // Recursive call...
            QVariantMap menuItem = doBuildMsBasicItem(subItem, parentMenuId, _ok, isSubItemCurrent);
            if (!_ok) {
                continue;
            }

            subItems << menuItem;

            if (isSubItemCurrent) {
                isCurrent = true;
            }
        }

        return buildMenuItem(menuId,
                             item.title,
                             isCurrent,
                             subItems);
    };

    QString menuId = MS_BASIC_SOUNDFONT_NAME.toQString() + "\\menu";

    QVariantList categoryItems;

    for (const MsBasicItem& category : MS_BASIC_PRESET_CATEGORIES) {
        bool ok = false;
        bool isCurrent = false;

        categoryItems << doBuildMsBasicItem(category, menuId, ok, isCurrent);
    }

    // Prepend the "Choose automatically" item
    categoryItems.prepend(buildSeparator());
    categoryItems.prepend(buildMenuItem(makeMenuResourceItemId(chooseAutomaticMeta.type, QString::fromStdString(chooseAutomaticMeta.id)),
                                        muse::qtrc("playback", "Choose automatically"),
                                        isCurrentSoundFont && !currentPreset.has_value()));

    return buildMenuItem(menuId,
                         MS_BASIC_SOUNDFONT_NAME,
                         isCurrentSoundFont,
                         categoryItems);
}

QVariantMap InputResourceItem::buildOtherSoundFontMenuItem(const muse::String& soundFont,
                                                           const audio::AudioResourceMetaList& availableResources) const
{
    // Group resources by bank, and use this to sort them
    std::map<int, std::map<int, AudioResourceMeta> > resourcesByBank;
    AudioResourceMeta chooseAutomaticMeta;

    for (const AudioResourceMeta& resourceMeta : availableResources) {
        bool bankOk = false, programOk = false;
        int presetBank = resourceMeta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
        int presetProgram = resourceMeta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);

        if (bankOk && programOk) {
            resourcesByBank[presetBank][presetProgram] = resourceMeta;
        } else {
            chooseAutomaticMeta = resourceMeta;
        }
    }

    const bool isCurrentSoundFont = soundFontIsCurrent(soundFont);
    const std::optional<midi::Program>& currentPreset = currentSoundFontProgram();

    QVariantList bankItems;

    for (const auto& bankPair : resourcesByBank) {
        bool isCurrentBank = isCurrentSoundFont && currentPreset.has_value() && currentPreset.value().bank == bankPair.first;

        QVariantList presetItems;

        for (const auto& presetPair : bankPair.second) {
            bool isCurrentPreset = isCurrentBank && currentPreset.value().program == presetPair.first;

            QString presetName = presetPair.second.attributeVal(PRESET_NAME_ATTRIBUTE);
            if (presetName.isEmpty()) {
                presetName = muse::qtrc("playback", "Preset %1").arg(presetPair.first);
            }

            QString itemId = makeMenuResourceItemId(AudioResourceType::FluidSoundfont, QString::fromStdString(presetPair.second.id));
            presetItems << buildMenuItem(itemId,
                                         presetName,
                                         isCurrentPreset);
        }

        bankItems << buildMenuItem(soundFont + u"\\" + muse::String::number(bankPair.first),
                                   muse::qtrc("playback", "Bank %1").arg(bankPair.first),
                                   isCurrentBank,
                                   presetItems);
    }

    // Prepend the "Choose automatically" item
    bankItems.prepend(buildSeparator());
    bankItems.prepend(buildMenuItem(makeMenuResourceItemId(chooseAutomaticMeta.type, QString::fromStdString(chooseAutomaticMeta.id)),
                                    muse::qtrc("playback", "Choose automatically"),
                                    isCurrentSoundFont && !currentPreset.has_value()));

    return buildMenuItem(soundFont + u"\\menu",
                         soundFont,
                         isCurrentSoundFont,
                         bankItems);
}

QVariantList InputResourceItem::buildSoundFontsFilteredList(const ResourceByVendorMap& resourcesByVendor, const QString& filterText) const
{
    const SoundFontMap soundFontMap = soundFontMapForResources(resourcesByVendor);
    IF_ASSERT_FAILED(!soundFontMap.empty()) {
        return QVariantList();
    }

    // Sort SoundFonts by name and add them to the menu
    std::vector<muse::String> soundFonts;
    soundFonts.reserve(soundFontMap.size());
    for (const auto& pair : soundFontMap) {
        soundFonts.push_back(pair.first);
    }

    std::sort(soundFonts.begin(), soundFonts.end(), [](const muse::String& s1, const muse::String& s2) {
        return muse::strings::lessThanCaseInsensitive(s1, s2);
    });

    QVariantList result;
    for (const muse::String& soundFont : soundFonts) {
        QVariantList list;
        if (soundFont == MS_BASIC_SOUNDFONT_NAME) {
            list = buildMsBasicFilteredList(soundFontMap.at(soundFont), filterText);
        } else {
            list = buildOtherSoundFontFilteredList(soundFont, soundFontMap.at(soundFont), filterText);
        }
        if (!list.empty() && !result.empty()) {
            result << buildSeparator();
        }
        result << list;
    }

    return result;
}

QVariantList InputResourceItem::buildMsBasicFilteredList(const muse::audio::AudioResourceMetaList& availableResources,
                                                         const QString& filterText) const
{
    std::map<midi::Program, AudioResourceMeta> resourcesByProgram;

    for (const AudioResourceMeta& resourceMeta : availableResources) {
        bool bankOk = false, programOk = false;
        int presetBank = resourceMeta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
        int presetProgram = resourceMeta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);
        if (bankOk && programOk) {
            resourcesByProgram[midi::Program(presetBank, presetProgram)] = resourceMeta;
        }
    }

    const std::optional<midi::Program>& currentPreset = currentSoundFontProgram();

    //! NOTE: This is called recursively for sub items...
    std::function<QVariantList(const MsBasicItem&)> doBuildMsBasicList = [&](const MsBasicItem& item) {
        QVariantList result;

        if (item.subItems.empty()) { // Base case - no more subitems to scan...
            auto it = resourcesByProgram.find(item.preset);
            if (it == resourcesByProgram.cend()) {
                LOGW() << "Preset specified in MS_BASIC_PRESET_CATEGORIES not found in SoundFont: bank " << item.preset.bank
                       << ", program " << item.preset.program;
                return result;
            }

            const AudioResourceMeta& resourceMeta = it->second;

            QString presetName = resourceMeta.attributeVal(PRESET_NAME_ATTRIBUTE);
            if (presetName.isEmpty()) {
                presetName = muse::qtrc("playback", "Bank %1, preset %2").arg(item.preset.bank).arg(item.preset.program);
            }

            // Temporary fix, see: https://github.com/musescore/MuseScore/issues/20142
            if (presetName.contains(muse::String(u"Expr."))) {
                return result;
            }

            const QString itemTitle = MS_BASIC_SOUNDFONT_NAME + " - " + presetName;
            if (!itemTitle.contains(filterText, Qt::CaseInsensitive)) {
                return result;
            }

            const QString itemId = makeMenuResourceItemId(resourceMeta.type, QString::fromStdString(resourceMeta.id));
            result << buildMenuItem(itemId, itemTitle, currentPreset.has_value() && currentPreset.value() == item.preset);

            return result;
        }

        QVariantList subItems;

        for (const MsBasicItem& subItem : item.subItems) {
            subItems << doBuildMsBasicList(subItem);
        }

        return subItems;
    };

    QVariantList result;

    for (const MsBasicItem& category : MS_BASIC_PRESET_CATEGORIES) {
        result << doBuildMsBasicList(category);
    }

    return result;
}

QVariantList InputResourceItem::buildOtherSoundFontFilteredList(const muse::String& soundFont,
                                                                const muse::audio::AudioResourceMetaList& availableResources,
                                                                const QString& filterText) const
{
    // Group resources by bank, and use this to sort them
    std::map<int, std::map<int, AudioResourceMeta> > resourcesByBank;

    for (const AudioResourceMeta& resourceMeta : availableResources) {
        bool bankOk = false, programOk = false;
        int presetBank = resourceMeta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
        int presetProgram = resourceMeta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);

        if (bankOk && programOk) {
            resourcesByBank[presetBank][presetProgram] = resourceMeta;
        }
    }

    const std::optional<midi::Program>& currentPreset = currentSoundFontProgram();

    QVariantList result;

    for (const auto& bankPair : resourcesByBank) {
        for (const auto& presetPair : bankPair.second) {
            QString presetName = presetPair.second.attributeVal(PRESET_NAME_ATTRIBUTE);
            if (presetName.isEmpty()) {
                presetName = muse::qtrc("playback", "Preset %1").arg(presetPair.first);
            }

            const QString itemTitle = soundFont.empty() ? presetName : soundFont + " - " + presetName;
            if (!itemTitle.contains(filterText, Qt::CaseInsensitive)) {
                continue;
            }

            const QString itemId = makeMenuResourceItemId(AudioResourceType::FluidSoundfont, QString::fromStdString(presetPair.second.id));
            result << buildMenuItem(itemId, itemTitle, currentPreset.has_value() && currentPreset.value().program == presetPair.first);
        }
    }

    return result;
}

void InputResourceItem::updateAvailableResources(const AudioResourceMetaList& availableResources)
{
    m_availableResourceMap.clear();

    for (const AudioResourceMeta& meta : availableResources) {
        ResourceByVendorMap& resourcesByVendor = m_availableResourceMap[meta.type];
        AudioResourceMetaList& resourcesMetaList = resourcesByVendor[meta.vendor];
        resourcesMetaList.push_back(meta);
    }

    for (auto& [type, resourcesByVendor] : m_availableResourceMap) {
        for (auto& [vendor, resourceMetaList] : resourcesByVendor) {
            sortResourcesList(resourceMetaList);
        }
    }
}
