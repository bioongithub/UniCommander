#include "ui/widget.h"
#include <algorithm>

namespace uc {

void Widget::addChild(std::shared_ptr<Widget> child)
{
    child->m_parent = shared_from_this();
    m_children.push_back(std::move(child));
}

void Widget::removeChild(Widget* child)
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const auto& p) { return p.get() == child; });
    if (it != m_children.end())
        m_children.erase(it);
}

void Widget::layout(int parentX, int parentY, int parentW, int parentH)
{
    switch (m_sizePolicy)
    {
        case SizePolicy::FillParent:
            m_x = parentX; m_y = parentY;
            m_w = parentW; m_h = parentH;
            break;

        case SizePolicy::Centered:
            m_x = parentX + (parentW - m_fixedW) / 2;
            m_y = parentY + (parentH - m_fixedH) / 2;
            m_w = m_fixedW; m_h = m_fixedH;
            break;

        case SizePolicy::Fixed:
            m_x = parentX; m_y = parentY;
            m_w = m_fixedW; m_h = m_fixedH;
            break;

        case SizePolicy::Relative:
            m_x = parentX + static_cast<int>(parentW * m_relX);
            m_y = parentY + static_cast<int>(parentH * m_relY);
            m_w = static_cast<int>(parentW * m_relW);
            m_h = static_cast<int>(parentH * m_relH);
            break;
    }

    for (auto& child : m_children)
        child->layout(m_x, m_y, m_w, m_h);
}

void Widget::paint(RenderContext& ctx)
{
    if (!m_visible) return;
    for (auto& child : m_children)
        child->paint(ctx);
}

Widget* Widget::hitTest(int x, int y)
{
    if (!m_visible) return nullptr;
    if (x < m_x || x >= m_x + m_w) return nullptr;
    if (y < m_y || y >= m_y + m_h) return nullptr;

    // Test children in reverse order: last child is topmost visually.
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
    {
        if (Widget* hit = (*it)->hitTest(x, y))
            return hit;
    }
    return this;
}

} // namespace uc
