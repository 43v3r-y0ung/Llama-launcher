#include "chartwidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <cmath>
#include <limits>

namespace {

constexpr int kPad = 8;
constexpr int kMiniSparkWidth = 48;
constexpr double kLineWidth = 2.0;

// Categorical slots 1-2 of the validated reference palette, stepped per theme.
const QColor kLight[] = {QColor(0x2a, 0x78, 0xd6), QColor(0xeb, 0x68, 0x34)};
const QColor kDark[] = {QColor(0x39, 0x87, 0xe5), QColor(0xd9, 0x59, 0x26)};

double niceCeil(double v)
{
    if (v <= 0)
        return 1;
    const double magnitude = std::pow(10.0, std::floor(std::log10(v)));
    for (double step : {1.0, 2.0, 2.5, 5.0, 10.0}) {
        if (v <= step * magnitude)
            return step * magnitude;
    }
    return 10 * magnitude;
}

QColor muted(const QPalette &pal)
{
    return pal.color(QPalette::PlaceholderText);
}

} // namespace

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void ChartWidget::setMode(Mode mode)
{
    m_mode = mode;
    setMouseTracking(mode == Mode::Full);
    updateGeometry();
    update();
}

void ChartWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void ChartWidget::setSeriesNames(const QStringList &names)
{
    m_names = names;
    updateGeometry();
    update();
}

void ChartWidget::setSubtitle(const QString &subtitle)
{
    if (subtitle == m_subtitle)
        return;
    const bool relayout = subtitle.isEmpty() != m_subtitle.isEmpty();
    m_subtitle = subtitle;
    if (relayout)
        updateGeometry();
    update();
}

void ChartWidget::setFormatter(std::function<QString(double)> f)
{
    m_formatter = std::move(f);
    update();
}

void ChartWidget::setMaximum(double max)
{
    m_maximum = max;
    update();
}

void ChartWidget::setCaption(const QString &caption)
{
    if (caption == m_caption)
        return;
    m_caption = caption;
    update();
}

void ChartWidget::setSizingText(const QString &text)
{
    m_sizingText = text;
    updateGeometry();
}

void ChartWidget::setCapacity(int samples)
{
    m_capacity = qMax(2, samples);
    update();
}

void ChartWidget::setData(const QList<QList<double>> &series)
{
    m_series = series;
    for (QList<double> &s : m_series) {
        if (s.size() > m_capacity)
            s = s.mid(s.size() - m_capacity);
    }
    update();
}

QSize ChartWidget::sizeHint() const
{
    if (m_mode == Mode::Mini)
        // left padding, caption, gap (1.5 pads), sparkline, right padding
        return {fontMetrics().horizontalAdvance(m_sizingText) + kMiniSparkWidth + 4 * kPad,
                fontMetrics().height() + kPad};
    return {240, 120};
}

QSize ChartWidget::minimumSizeHint() const
{
    if (m_mode == Mode::Mini)
        return sizeHint();
    const QFontMetrics fm = fontMetrics();
    int h = 2 * kPad + fm.height() + 40;
    if (m_names.size() > 1)
        h += fm.height();
    if (!m_subtitle.isEmpty())
        h += fm.height();
    return {160, h};
}

bool ChartWidget::isDark() const
{
    return palette().color(QPalette::Window).lightness() < 128;
}

QColor ChartWidget::seriesColor(int index) const
{
    const QColor *set = isDark() ? kDark : kLight;
    return set[qBound(0, index, 1)];
}

double ChartWidget::effectiveMaximum() const
{
    if (m_maximum > 0)
        return m_maximum;
    double top = 0;
    for (const QList<double> &s : m_series)
        for (double v : s)
            if (!std::isnan(v))
                top = qMax(top, v);
    return niceCeil(top * 1.1);
}

QRectF ChartWidget::plotRect() const
{
    const QFontMetrics fm = fontMetrics();
    QRectF r = QRectF(rect()).adjusted(kPad, kPad, -kPad, -kPad);
    if (m_mode == Mode::Mini) {
        r.setLeft(r.right() - kMiniSparkWidth);
        return r.adjusted(0, 1, 0, -1);
    }
    r.setTop(r.top() + fm.height() + 4);           // title row
    if (m_names.size() > 1)
        r.setTop(r.top() + fm.height());            // legend row
    if (!m_subtitle.isEmpty())
        r.setBottom(r.bottom() - fm.height() - 2);
    return r;
}

double ChartWidget::xFor(int index, int count, const QRectF &plot) const
{
    const int slot = m_capacity - count + index; // newest sample on the right edge
    return plot.left() + plot.width() * slot / double(m_capacity - 1);
}

int ChartWidget::indexAt(double x, int count, const QRectF &plot) const
{
    if (count <= 0 || plot.width() <= 0)
        return -1;
    const int slot = qRound((x - plot.left()) / plot.width() * (m_capacity - 1));
    const int index = slot - (m_capacity - count);
    return (index >= 0 && index < count) ? index : -1;
}

QString ChartWidget::format(double v) const
{
    if (std::isnan(v))
        return QStringLiteral("—");
    return m_formatter ? m_formatter(v) : QString::number(v);
}

void ChartWidget::paintSeries(QPainter &p, const QRectF &plot, double max, bool fill) const
{
    auto yFor = [&](double v) { return plot.bottom() - plot.height() * qBound(0.0, v / max, 1.0); };

    for (int si = m_series.size() - 1; si >= 0; --si) { // first series painted last, on top
        const QList<double> &s = m_series.at(si);
        const int n = s.size();
        QPainterPath line;
        QList<QPainterPath> areas;
        QPainterPath area;
        bool open = false;
        double lastX = 0;
        for (int i = 0; i < n; ++i) {
            const double v = s.at(i);
            if (std::isnan(v)) {
                if (open) {
                    area.lineTo(lastX, plot.bottom());
                    area.closeSubpath();
                    areas << area;
                    area = QPainterPath();
                }
                open = false;
                continue;
            }
            const QPointF pt(xFor(i, n, plot), yFor(v));
            if (!open) {
                line.moveTo(pt);
                area.moveTo(pt.x(), plot.bottom());
                area.lineTo(pt);
                open = true;
            } else {
                line.lineTo(pt);
                area.lineTo(pt);
            }
            lastX = pt.x();
        }
        if (open) {
            area.lineTo(lastX, plot.bottom());
            area.closeSubpath();
            areas << area;
        }

        const QColor c = seriesColor(si);
        if (fill && si == 0) {
            QColor f = c;
            f.setAlphaF(isDark() ? 0.22 : 0.16);
            for (const QPainterPath &a : areas)
                p.fillPath(a, f);
        }
        QPen pen(c, kLineWidth);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(line);
    }
}

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette pal = palette();
    const QFontMetrics fm = fontMetrics();
    const double max = effectiveMaximum();
    const QRectF plot = plotRect();

    // Card
    const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.setBrush(pal.color(QPalette::Base));
    p.drawRoundedRect(card, 6, 6);

    auto latest = [](const QList<double> &s) {
        for (int i = s.size() - 1; i >= 0; --i)
            if (!std::isnan(s.at(i)))
                return s.at(i);
        return std::numeric_limits<double>::quiet_NaN();
    };

    if (m_mode == Mode::Mini) {
        const double v = m_series.isEmpty() ? std::numeric_limits<double>::quiet_NaN() : latest(m_series.first());
        p.setPen(pal.color(QPalette::Text));
        const QRectF textRect(kPad, 0, plot.left() - 1.5 * kPad, height());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                   fm.elidedText(m_caption.isEmpty() ? m_title + QLatin1Char(' ') + format(v) : m_caption,
                                 Qt::ElideRight, int(textRect.width())));
        paintSeries(p, plot, max, true);
        return;
    }

    // Title row: title on the left, values (current or hovered) on the right.
    QFont bold = font();
    bold.setBold(true);
    p.setFont(bold);
    p.setPen(pal.color(QPalette::Text));
    const QRectF titleRect(kPad, kPad, width() - 2 * kPad, fm.height());
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    const int titleWidth = QFontMetrics(bold).horizontalAdvance(m_title);
    p.setFont(font());

    QStringList values;
    for (const QList<double> &s : m_series) {
        const bool hovering = m_hoverIndex >= 0 && m_hoverIndex < s.size();
        values << format(hovering ? s.at(m_hoverIndex) : latest(s));
    }
    QString valueText = values.join(QStringLiteral(" · "));
    if (m_hoverIndex >= 0 && !m_series.isEmpty()) {
        const int ago = m_series.first().size() - 1 - m_hoverIndex;
        valueText = tr("%1 s ago: ").arg(ago) + valueText;
    }
    p.drawText(titleRect.adjusted(titleWidth + kPad, 0, 0, 0), Qt::AlignRight | Qt::AlignVCenter,
               fm.elidedText(valueText, Qt::ElideLeft, int(titleRect.width()) - titleWidth - kPad));

    // Legend: identity never relies on color alone.
    if (m_names.size() > 1) {
        double x = kPad;
        const double y = titleRect.bottom() + 2;
        for (int i = 0; i < m_names.size(); ++i) {
            const QRectF swatch(x, y + fm.height() / 2.0 - 1.5, 12, 3);
            p.setPen(Qt::NoPen);
            p.setBrush(seriesColor(i));
            p.drawRoundedRect(swatch, 1.5, 1.5);
            x += 16;
            p.setPen(muted(pal));
            p.drawText(QPointF(x, y + fm.ascent()), m_names.at(i));
            x += fm.horizontalAdvance(m_names.at(i)) + 14;
        }
    }

    // Grid: baseline, middle and top, recessive.
    QColor grid = pal.color(QPalette::Mid);
    grid.setAlphaF(0.5);
    p.setPen(QPen(grid, 1));
    p.drawLine(QPointF(plot.left(), plot.bottom()), QPointF(plot.right(), plot.bottom()));
    QPen dashed(grid, 1, Qt::DashLine);
    p.setPen(dashed);
    p.drawLine(QPointF(plot.left(), plot.top()), QPointF(plot.right(), plot.top()));
    p.drawLine(QPointF(plot.left(), plot.center().y()), QPointF(plot.right(), plot.center().y()));
    paintSeries(p, plot, max, true);

    // Axis labels on top of the data, on a surface-colored chip so lines never run through them:
    // the y maximum at the top and the x span at the bottom, both on the left (oldest, emptiest side).
    QFont small = font();
    if (small.pointSizeF() > 0)
        small.setPointSizeF(small.pointSizeF() * 0.85);
    p.setFont(small);
    const QFontMetrics sfm(small);
    auto axisLabel = [&](const QString &text, double y) {
        const QRectF r(plot.left(), y, sfm.horizontalAdvance(text) + 6, sfm.height());
        p.fillRect(r, pal.color(QPalette::Base));
        p.setPen(muted(pal));
        p.drawText(r, Qt::AlignCenter, text);
    };
    axisLabel(format(max), plot.top() + 1);
    const int minutes = m_capacity / 60;
    axisLabel(minutes > 0 ? tr("%n min", nullptr, minutes) : tr("%n s", nullptr, m_capacity),
              plot.bottom() - sfm.height() - 1);
    p.setFont(font());

    // Hover crosshair
    if (m_hoverIndex >= 0 && !m_series.isEmpty()) {
        const int n = m_series.first().size();
        const double x = xFor(m_hoverIndex, n, plot);
        p.setPen(QPen(muted(pal), 1));
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        for (int si = 0; si < m_series.size(); ++si) {
            const QList<double> &s = m_series.at(si);
            if (m_hoverIndex >= s.size() || std::isnan(s.at(m_hoverIndex)))
                continue;
            const double y = plot.bottom() - plot.height() * qBound(0.0, s.at(m_hoverIndex) / max, 1.0);
            p.setPen(QPen(pal.color(QPalette::Base), 2)); // surface ring
            p.setBrush(seriesColor(si));
            p.drawEllipse(QPointF(x, y), 4, 4);
        }
    }

    if (!m_subtitle.isEmpty()) {
        p.setPen(muted(pal));
        p.drawText(QRectF(kPad, plot.bottom() + 2, width() - 2 * kPad, fm.height()), Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(m_subtitle, Qt::ElideRight, width() - 2 * kPad));
    }
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mode != Mode::Full || m_series.isEmpty())
        return;
    const QRectF plot = plotRect();
    int index = -1;
    if (event->position().y() >= plot.top() - 4 && event->position().y() <= plot.bottom() + 4)
        index = indexAt(event->position().x(), m_series.first().size(), plot);
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        update();
    }
}

void ChartWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverIndex != -1) {
        m_hoverIndex = -1;
        update();
    }
}

void ChartWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
        update(); // colors follow the light/dark theme
}
