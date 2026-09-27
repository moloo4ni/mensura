#include "mensurawindow.h"

#include "mensurapanel.h"

#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin::Mensura {
MensuraWindow::MensuraWindow(MensuraController* controller, QWidget* parent)
    : FyWidget{parent}
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new MensuraPanel(controller, this));
}

QString MensuraWindow::name() const
{
    return tr("Mensura");
}

QString MensuraWindow::layoutName() const
{
    return u"Mensura"_s;
}
} // namespace Fooyin::Mensura
