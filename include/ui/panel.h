#pragma once
#include <string>

// Abstract base class for a file-browser panel (left or right pane).
class Panel
{
public:
    virtual ~Panel() = default;

    virtual void        setPath(const std::string& path) = 0;
    virtual std::string getPath() const = 0;
    virtual void        refresh() = 0;   // re-reads directory contents
    virtual void        render()  = 0;   // redraws the panel
};
