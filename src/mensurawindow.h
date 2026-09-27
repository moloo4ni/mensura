#pragma once

#include <gui/fywidget.h>

namespace Fooyin::Mensura {
class MensuraController;

//! FyWidget wrapper so the panel can be shown with FyWidget::showStandaloneWindow.
class MensuraWindow : public FyWidget
{
    Q_OBJECT

public:
    explicit MensuraWindow(MensuraController* controller, QWidget* parent = nullptr);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString layoutName() const override;
};
} // namespace Fooyin::Mensura
