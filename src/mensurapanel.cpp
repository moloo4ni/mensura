#include "mensurapanel.h"

#include "beatgrid.h"
#include "beatindicator.h"
#include "mensuracontroller.h"
#include "sharedstate.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
namespace {
constexpr int IndicatorIntervalMs = 16; // ~60 Hz
// Half the displayed resolution (1 decimal): smaller differences look identical in the box
constexpr double BpmDisplayTolerance = 0.05;
}

MensuraPanel::MensuraPanel(MensuraController* controller, QWidget* parent)
    : QWidget{parent}
    , m_controller{controller}
    , m_enabled{new QCheckBox(tr("Enable metronome"), this)}
    , m_bpm{new QDoubleSpinBox(this)}
    , m_source{new QLabel(this)}
    , m_mode{new QComboBox(this)}
    , m_tap{new QPushButton(tr("TAP"), this)}
    , m_indicator{new BeatIndicator(this)}
    , m_beats{new QSpinBox(this)}
    , m_sound{new QComboBox(this)}
    , m_volume{new QSlider(Qt::Horizontal, this)}
    , m_volumeLabel{new QLabel(this)}
    , m_offset{new QSpinBox(this)}
    , m_resetPhase{new QPushButton(tr("Reset"), this)}
    , m_warning{new QLabel(tr("⚠ Mensura is not in the DSP chain. Add it in Settings → DSP → Per-track."), this)}
    , m_timer{new QTimer(this)}
{
    m_enabled->setObjectName(u"enabled"_s);
    m_bpm->setObjectName(u"bpm"_s);
    m_source->setObjectName(u"source"_s);
    m_mode->setObjectName(u"mode"_s);
    m_tap->setObjectName(u"tap"_s);
    m_indicator->setObjectName(u"indicator"_s);
    m_beats->setObjectName(u"beats"_s);
    m_sound->setObjectName(u"sound"_s);
    m_volume->setObjectName(u"volume"_s);
    m_volumeLabel->setObjectName(u"volumeLabel"_s);
    m_offset->setObjectName(u"offset"_s);
    m_resetPhase->setObjectName(u"resetPhase"_s);
    m_warning->setObjectName(u"warning"_s);

    m_bpm->setRange(MinBpm, MaxBpm);
    m_bpm->setDecimals(1);
    m_bpm->setSingleStep(1.0);
    m_bpm->setKeyboardTracking(false);
    m_bpm->setPrefix(u"♩ "_s);
    QFont bpmFont = m_bpm->font();
    if(bpmFont.pointSizeF() > 0) {
        bpmFont.setPointSizeF(bpmFont.pointSizeF() * 1.5);
    }
    else {
        bpmFont.setPixelSize(bpmFont.pixelSize() * 3 / 2);
    }
    m_bpm->setFont(bpmFont);

    m_mode->addItem(tr("Auto"));   // BpmMode::Auto
    m_mode->addItem(tr("Manual")); // BpmMode::Manual

    m_tap->setMinimumHeight(m_tap->sizeHint().height() * 2);

    m_beats->setRange(MinBeatsPerBar, MaxBeatsPerBar);

    m_sound->addItem(tr("Click")); // ClickSound order
    m_sound->addItem(tr("Wood"));
    m_sound->addItem(tr("Beep"));

    m_volume->setRange(static_cast<int>(MensuraConfig::MinVolumeDb), static_cast<int>(MensuraConfig::MaxVolumeDb));

    m_offset->setRange(-MensuraConfig::MaxPhaseOffsetMs, MensuraConfig::MaxPhaseOffsetMs);
    m_offset->setSuffix(tr(" ms"));
    m_offset->setKeyboardTracking(false);

    m_warning->setWordWrap(true);
    m_warning->hide();

    auto* tempoRow = new QHBoxLayout();
    tempoRow->addWidget(m_bpm, 1);
    tempoRow->addWidget(m_source);
    tempoRow->addSpacing(12);
    tempoRow->addWidget(new QLabel(tr("Mode:"), this));
    tempoRow->addWidget(m_mode);

    auto* volumeRow = new QHBoxLayout();
    volumeRow->addWidget(m_volume, 1);
    volumeRow->addWidget(m_volumeLabel);

    auto* offsetRow = new QHBoxLayout();
    offsetRow->addWidget(m_offset, 1);
    offsetRow->addWidget(m_resetPhase);

    auto* form = new QFormLayout();
    form->addRow(tr("Beats per bar:"), m_beats);
    form->addRow(tr("Sound:"), m_sound);
    form->addRow(tr("Volume:"), volumeRow);
    form->addRow(tr("Phase offset:"), offsetRow);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_enabled);
    layout->addLayout(tempoRow);
    layout->addWidget(m_tap);
    layout->addWidget(m_indicator);
    layout->addLayout(form);
    layout->addWidget(m_warning);
    layout->addStretch();

    connect(m_enabled, &QCheckBox::toggled, m_controller, &MensuraController::setEnabled);
    connect(m_bpm, &QDoubleSpinBox::valueChanged, m_controller, &MensuraController::setManualBpm);
    connect(m_mode, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_controller->setMode(static_cast<BpmMode>(index)); });
    connect(m_tap, &QPushButton::pressed, this, &MensuraPanel::tap);
    connect(m_beats, &QSpinBox::valueChanged, m_controller, &MensuraController::setBeatsPerBar);
    connect(m_sound, &QComboBox::currentIndexChanged, this,
            [this](int index) { m_controller->setSound(static_cast<ClickSound>(index)); });
    connect(m_volume, &QSlider::valueChanged, this, [this](int db) { m_controller->setVolumeDb(db); });
    connect(m_offset, &QSpinBox::valueChanged, m_controller, &MensuraController::setPhaseOffsetMs);
    connect(m_resetPhase, &QPushButton::clicked, m_controller, &MensuraController::resetPhase);

    auto* tapShortcut = new QShortcut(QKeySequence{Qt::Key_T}, this);
    tapShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    tapShortcut->setAutoRepeat(false);
    // Spin boxes claim printable keys as shortcut overrides; T is never valid input there
    m_bpm->installEventFilter(this);
    m_offset->installEventFilter(this);
    connect(tapShortcut, &QShortcut::activated, this, &MensuraPanel::tap);

    connect(m_controller, &MensuraController::stateChanged, this, &MensuraPanel::refresh);
    connect(m_controller, &MensuraController::nodeMissingChanged, this, &MensuraPanel::refresh);

    m_timer->setInterval(IndicatorIntervalMs);
    connect(m_timer, &QTimer::timeout, this, [this] { updateIndicator(SharedState::nowNs()); });

    refresh();
}

void MensuraPanel::refresh()
{
    const MensuraConfig& config = m_controller->config();
    const ResolvedTempo tempo   = m_controller->tempo();
    const bool fromTag          = tempo.source == BpmSource::Tag;

    const QSignalBlocker enabledBlocker{m_enabled};
    const QSignalBlocker bpmBlocker{m_bpm};
    const QSignalBlocker modeBlocker{m_mode};
    const QSignalBlocker beatsBlocker{m_beats};
    const QSignalBlocker soundBlocker{m_sound};
    const QSignalBlocker volumeBlocker{m_volume};
    const QSignalBlocker offsetBlocker{m_offset};

    m_enabled->setChecked(config.enabled);

    // Writing an unchanged value would discard half-typed text (keyboard tracking is off)
    if(std::abs(m_bpm->value() - tempo.bpm) >= BpmDisplayTolerance) {
        m_bpm->setValue(tempo.bpm);
    }
    QPalette bpmPalette = palette();
    if(fromTag) {
        bpmPalette.setColor(QPalette::Text, bpmPalette.color(QPalette::Disabled, QPalette::Text));
    }
    m_bpm->setPalette(bpmPalette);
    m_source->setText(fromTag ? tr("tag") : tr("manual"));

    m_mode->setCurrentIndex(static_cast<int>(config.mode));
    if(m_beats->value() != config.beatsPerBar) {
        m_beats->setValue(config.beatsPerBar);
    }
    m_sound->setCurrentIndex(static_cast<int>(config.sound));

    const auto volumeDb = static_cast<int>(std::lround(config.volumeDb));
    m_volume->setValue(volumeDb);
    m_volumeLabel->setText(tr("%1 dB").arg(volumeDb));

    if(m_offset->value() != config.phaseOffsetMs) {
        m_offset->setValue(config.phaseOffsetMs);
    }
    m_tap->setEnabled(m_controller->isPlaying());
    m_indicator->setBeatCount(config.beatsPerBar);
    m_warning->setVisible(m_controller->nodeMissing());
    updateTimer();
}

void MensuraPanel::updateIndicator(int64_t nowNs)
{
    if(!m_controller->isPlaying()) {
        m_indicator->setCurrentBeat(-1);
        return;
    }
    const BeatGrid grid{m_controller->tempo().bpm, m_controller->phaseNs(), m_controller->config().beatsPerBar};
    const int64_t beat = grid.beatAtOrBefore(std::llround(m_controller->positionMs(nowNs) * 1e6));
    m_indicator->setCurrentBeat(grid.indexInBar(beat));
}

bool MensuraPanel::eventFilter(QObject* watched, QEvent* event)
{
    if(event->type() == QEvent::ShortcutOverride) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_T && keyEvent->modifiers() == Qt::NoModifier) {
            event->ignore(); // not claimed by the spin box, so the T shortcut fires
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MensuraPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_shown = true;
    updateTimer();
}

void MensuraPanel::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_shown = false; // also for spontaneous hides (minimise), where isVisible() stays true
    updateTimer();
}

void MensuraPanel::updateTimer()
{
    if(m_shown && m_controller->isPlaying()) {
        if(!m_timer->isActive()) {
            m_timer->start();
        }
    }
    else {
        m_timer->stop();
        m_indicator->setCurrentBeat(-1);
    }
}

void MensuraPanel::tap()
{
    if(m_tap->isEnabled()) {
        m_controller->tap(SharedState::nowNs());
    }
}
} // namespace Fooyin::Mensura
