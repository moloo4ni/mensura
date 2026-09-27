#include "temporesolver.h"

#include "mensuraparams.h"

#include <core/track.h>

#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
std::optional<double> parseBpmTag(const QString& text)
{
    // A minus counts as a sign only when it does not follow a letter or digit,
    // so "-120" is negative but "BPM-128" is 128.
    static const QRegularExpression number{uR"(((?:(?<!\w)-)?\d+(?:[.,]\d+)?))"_s};

    const auto match = number.match(text);
    if(!match.hasMatch()) {
        return {};
    }

    QString digits = match.captured(1);
    digits.replace(u',', u'.');

    bool ok{false};
    const double value = digits.toDouble(&ok);
    if(!ok || value <= 0.0) {
        return {};
    }
    return clampBpm(value);
}

std::optional<double> trackBpm(const Track& track)
{
    const QStringList values = track.extraTag(u"BPM"_s);
    for(const QString& value : values) {
        if(const auto bpm = parseBpmTag(value)) {
            return bpm;
        }
    }
    return {};
}

ResolvedTempo resolveTempo(BpmMode mode, std::optional<double> tagBpm, double manualBpm)
{
    if(mode == BpmMode::Auto && tagBpm) {
        return {.bpm = clampBpm(*tagBpm), .source = BpmSource::Tag};
    }
    return {.bpm = clampBpm(manualBpm), .source = BpmSource::Manual};
}
} // namespace Fooyin::Mensura
