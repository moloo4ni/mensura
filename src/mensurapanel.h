#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;

namespace Fooyin::Mensura {
class BeatIndicator;
class MensuraController;

//! All controls of the Mensura window. Reads and writes state only through the controller.
class MensuraPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MensuraPanel(MensuraController* controller, QWidget* parent = nullptr);

    //! Copies controller state into the widgets without feeding it back.
    void refresh();
    //! Lights the beat at the interpolated position; nothing while not playing.
    void updateIndicator(int64_t nowNs);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void tap();
    //! The indicator timer runs only while the panel is visible and playback is running.
    void updateTimer();

    MensuraController* m_controller;

    QCheckBox* m_enabled;
    QDoubleSpinBox* m_bpm;
    QLabel* m_source;
    QComboBox* m_mode;
    QPushButton* m_tap;
    BeatIndicator* m_indicator;
    QSpinBox* m_beats;
    QComboBox* m_sound;
    QComboBox* m_accent;
    QSlider* m_volume;
    QLabel* m_volumeLabel;
    QSpinBox* m_offset;
    QPushButton* m_resetPhase;
    QLabel* m_warning;
    QTimer* m_timer;
    bool m_shown{false}; // isVisible() stays true while minimised
};
} // namespace Fooyin::Mensura
