#pragma once

#include <QWidget>

namespace Fooyin::Mensura {
//! One dot per beat of the bar; the current beat is lit, beat 0 in the accent colour.
class BeatIndicator : public QWidget
{
    Q_OBJECT

public:
    static constexpr int MaxDots = 8; // above this a compact row of segments is drawn

    explicit BeatIndicator(QWidget* parent = nullptr);

    void setBeatCount(int count);
    //! -1 lights nothing.
    void setCurrentBeat(int beat);

    [[nodiscard]] int beatCount() const;
    [[nodiscard]] int currentBeat() const;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_count{4};
    int m_current{-1};
};
} // namespace Fooyin::Mensura
