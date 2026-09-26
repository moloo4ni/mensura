#include "beatindicator.h"

#include "mensuraparams.h"

#include <QPainter>

#include <algorithm>

namespace Fooyin::Mensura {
BeatIndicator::BeatIndicator(QWidget* parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void BeatIndicator::setBeatCount(int count)
{
    count = std::clamp(count, MinBeatsPerBar, MaxBeatsPerBar);
    if(count == m_count) {
        return;
    }
    m_count = count;
    if(m_current >= m_count) {
        m_current = -1;
    }
    update();
}

void BeatIndicator::setCurrentBeat(int beat)
{
    beat = (beat >= 0 && beat < m_count) ? beat : -1;
    if(beat == m_current) {
        return;
    }
    m_current = beat;
    update();
}

int BeatIndicator::beatCount() const
{
    return m_count;
}

int BeatIndicator::currentBeat() const
{
    return m_current;
}

QSize BeatIndicator::sizeHint() const
{
    return {240, 28};
}

QSize BeatIndicator::minimumSizeHint() const
{
    return {60, 16};
}

void BeatIndicator::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    const QRectF area   = QRectF{rect()}.adjusted(2, 2, -2, -2);
    const double slot   = area.width() / m_count;
    const double centre = area.center().y();

    for(int beat = 0; beat < m_count; ++beat) {
        QColor colour = palette().color(QPalette::Mid);
        if(beat == m_current) {
            colour = (beat == 0 && m_count > 1) ? palette().color(QPalette::Highlight)
                                                : palette().color(QPalette::WindowText);
        }
        painter.setBrush(colour);

        if(m_count <= MaxDots) {
            const double radius = std::min(slot * 0.6, area.height()) / 2.0;
            painter.drawEllipse(QPointF{area.left() + slot * (beat + 0.5), centre}, radius, radius);
        }
        else {
            const QRectF segment{area.left() + slot * beat + 1.0, centre - 4.0, slot - 2.0, 8.0};
            painter.drawRoundedRect(segment, 3.0, 3.0);
        }
    }
}
} // namespace Fooyin::Mensura
