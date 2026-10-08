#pragma once

#include <QString>
#include "Project.h"

namespace CodeGen {

/**
 * @brief Generates runtime data binding code connecting DataSources and UI widgets
 * without coupling widgets directly to target hardware drivers.
 */
class BindingLayerGenerator {
public:
    static QString generateBindingsHeader(const Project* project);
    static QString generateBindingsSource(const Project* project);
    static QString generateQulBindings(const Project* project);
};

} // namespace CodeGen
