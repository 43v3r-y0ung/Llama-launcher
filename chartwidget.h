#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QList>
#include <QStringList>
#include <QWidget>
#include <functional>

// A lightweight time-series chart drawn with QPainter: one shared y-axis, up to two series,
// newest sample at the right edge. NaN values leave gaps.
//
// Full mode: title, legend (for two series), current values, recessive grid and a hover
// crosshair that shows the values under the cursor.
// Mini mode: a sparkline with a short caption, for the status bar.
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Mode { Full, Mini };

    explicit ChartWidget(QWidget *parent = nullptr);

    void setMode(Mode mode);
    void setTitle(const QString &title);
    void setSeriesNames(const QStringList &names);
    void setSubtitle(const QString &subtitle);           // extra line under the plot (full mode)
    void setFormatter(std::function<QString(double)> f); // value -> text, e.g. "34 %"
    void setMaximum(double max);                         // fixed y-range 0..max; <= 0 scales automatically
    void setCapacity(int samples);                       // x-range in samples
    void setCaption(const QString &caption);             // mini mode: text instead of "title value"
    void setSizingText(const QString &text);             // mini mode: widest caption to reserve room for
    void setData(const QList<QList<double>> &series);    // oldest first, one value per second

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    QColor seriesColor(int index) const;
    bool isDark() const;
    double effectiveMaximum() const;
    QRectF plotRect() const;
    double xFor(int index, int count, const QRectF &plot) const;
    int indexAt(double x, int count, const QRectF &plot) const;
    QString format(double v) const;
    void paintSeries(QPainter &p, const QRectF &plot, double max, bool fill) const;

    Mode m_mode = Mode::Full;
    QString m_title;
    QStringList m_names;
    QString m_subtitle;
    std::function<QString(double)> m_formatter;
    double m_maximum = 0;
    int m_capacity = 300;
    QString m_caption;
    QString m_sizingText = QStringLiteral("GPU 100 %");
    QList<QList<double>> m_series;
    int m_hoverIndex = -1;
};

#endif // CHARTWIDGET_H
