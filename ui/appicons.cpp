#include "ui/appicons.h"
#include <QApplication>
#include <QIconEngine>
#include <QMenuBar>
#include <QPainter>
#include <QPalette>
#include <QProxyStyle>
#include <QStyleOptionMenuItem>

namespace {
// QMenuBar's standard painting replaces a title with its icon. Reserve room
// for both while leaving menu colors and selection rendering to the OS style.
class MenuStyle final : public QProxyStyle {
  public:
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget) const override {
        const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (type == CT_MenuBarItem && item && !item->icon.isNull()) {
            auto textItem = *item;
            textItem.icon = QIcon();
            return QProxyStyle::sizeFromContents(type, &textItem,
                item->fontMetrics.size(Qt::TextShowMnemonic, item->text), widget) + QSize(22, 0);
        }
        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override {
        const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (element == CE_MenuBarItem && item && !item->icon.isNull()) {
            auto background = *item;
            background.icon = QIcon();
            background.text.clear();
            QProxyStyle::drawControl(element, &background, painter, widget);
            auto textItem = *item;
            textItem.icon = QIcon();
            textItem.rect.adjust(22, 0, 0, 0);
            QProxyStyle::drawControl(element, &textItem, painter, widget);
            item->icon.paint(painter,
                             QRect(item->rect.left() + 5, item->rect.center().y() - 8, 16, 16));
            return;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

class Engine : public QIconEngine {
  public:
    explicit Engine(AppIcons::Kind kind) : m_kind(kind) {}
    QIconEngine *clone() const override {
        return new Engine(m_kind);
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter p(&result);
        paint(&p, QRect(QPoint(), size), mode, state);
        return result;
    }
    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State) override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->translate(rect.center());
        p->scale(rect.width() / 24.0, rect.height() / 24.0);
        p->translate(-12, -12);
        p->setPen(QPen(
            qApp->palette().color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
                                  QPalette::WindowText),
            1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        using K = AppIcons::Kind;
        auto line = [p](int a, int b, int c, int d) { p->drawLine(a, b, c, d); };
        switch (m_kind) {
        case K::File:
            p->drawRoundedRect(5, 3, 14, 18, 2, 2);
            line(8, 8, 16, 8);
            line(8, 12, 16, 12);
            break;
        case K::New:
            p->drawRect(4, 3, 12, 18);
            line(13, 14, 21, 14);
            line(17, 10, 17, 18);
            break;
        case K::Save:
            p->drawRect(3, 3, 18, 18);
            p->drawRect(7, 3, 10, 6);
            p->drawRect(7, 14, 10, 7);
            break;
        case K::Open:
            p->drawPolyline(QPolygonF{QPointF(3, 19), QPointF(3, 5), QPointF(9, 5), QPointF(12, 8),
                                      QPointF(21, 8)});
            p->drawPolygon(
                QPolygonF{QPointF(3, 19), QPointF(6, 10), QPointF(22, 10), QPointF(19, 19)});
            break;
        case K::Image:
            p->drawRoundedRect(3, 3, 18, 18, 2, 2);
            p->drawEllipse(6, 6, 4, 4);
            p->drawPolyline(QPolygonF{QPointF(4, 18), QPointF(10, 12), QPointF(14, 16),
                                      QPointF(18, 10), QPointF(21, 14)});
            break;
        case K::Import:
        case K::Export:
            p->drawEllipse(3, 3, 18, 5);
            line(3, 6, 3, 19);
            line(21, 6, 21, 19);
            p->drawArc(3, 16, 18, 6, 180 * 16, 180 * 16);
            if (m_kind == K::Import) {
                line(12, 8, 12, 18);
                line(8, 14, 12, 18);
                line(16, 14, 12, 18);
            } else {
                line(12, 18, 12, 8);
                line(8, 12, 12, 8);
                line(16, 12, 12, 8);
            }
            break;
        case K::Exit:
            line(3, 3, 3, 21);
            line(3, 3, 12, 3);
            line(3, 21, 12, 21);
            line(9, 12, 22, 12);
            line(17, 7, 22, 12);
            line(17, 17, 22, 12);
            break;
        case K::Settings:
            for (int x : {6, 12, 18})
                line(x, 3, x, 21);
            p->drawEllipse(4, 6, 4, 4);
            p->drawEllipse(10, 14, 4, 4);
            p->drawEllipse(16, 9, 4, 4);
            break;
        case K::Language:
            p->drawEllipse(3, 3, 18, 18);
            p->drawEllipse(8, 3, 8, 18);
            line(3, 12, 21, 12);
            break;
        case K::English:
        case K::Russian: {
            QFont f = p->font();
            f.setPixelSize(11);
            f.setBold(true);
            p->setFont(f);
            p->drawRoundedRect(1, 4, 22, 16, 2, 2);
            p->drawText(QRect(1, 4, 22, 16), Qt::AlignCenter, m_kind == K::English ? "EN" : "RU");
            break;
        }
        case K::Log:
            p->drawRect(4, 3, 16, 18);
            for (int y : {7, 12, 17}) {
                line(7, y, 8, y);
                line(11, y, 17, y);
            }
            break;
        case K::Add:
            p->drawEllipse(3, 3, 18, 18);
            line(7, 12, 17, 12);
            line(12, 7, 12, 17);
            break;
        case K::Remove:
            line(4, 6, 20, 6);
            line(9, 3, 15, 3);
            p->drawRect(6, 6, 12, 15);
            line(10, 10, 10, 17);
            line(14, 10, 14, 17);
            break;
        case K::Swap:
            line(3, 7, 21, 7);
            line(17, 3, 21, 7);
            line(17, 11, 21, 7);
            line(21, 17, 3, 17);
            line(7, 13, 3, 17);
            line(7, 21, 3, 17);
            break;
        case K::Copy:
            p->drawRect(8, 7, 12, 14);
            p->drawPolyline(
                QPolygonF{QPointF(5, 17), QPointF(3, 17), QPointF(3, 3), QPointF(15, 3)});
            break;
        case K::Clear:
            p->drawPolygon(QPolygonF{QPointF(4, 15), QPointF(13, 4), QPointF(21, 11),
                                     QPointF(13, 21), QPointF(10, 21)});
            line(8, 11, 17, 18);
            break;
        }
        p->restore();
    }

  private:
    AppIcons::Kind m_kind;
};
} // namespace
QIcon AppIcons::icon(Kind kind) {
    return QIcon(new Engine(kind));
}
QIcon AppIcons::applicationIcon() {
    return QIcon(QStringLiteral(":/icons/m2mtools.png"));
}
QIcon AppIcons::logWindowIcon() {
    return QIcon(QStringLiteral(":/icons/event-log.png"));
}

void AppIcons::configureMenuBar(QMenuBar *bar) {
    auto *style = new MenuStyle;
    style->setParent(bar);
    bar->setStyle(style);
}
