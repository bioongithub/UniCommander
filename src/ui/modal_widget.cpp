#include "ui/modal_widget.h"

namespace uc {

void ModalWidget::beginModal()
{
    setVisible(true);
    m_loop.run(this);
}

void ModalWidget::endModal()
{
    setVisible(false);
    requestRepaint();   // ensure the overlay is erased on the next frame
    m_loop.stop();
}

} // namespace uc
