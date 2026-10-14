#pragma once

#include "IRTypes.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/UIComponent.h"
#include "models/DataSource.h"
#include "models/DataBinding.h"

#include <memory>

namespace IR {

class ProjectToIRAdapter {
public:
    static IRProject convert(const Project& project);

private:
    static IRScreen convertScreen(const Screen& screen);
    static IRWidget convertComponent(const UIComponent& component);
    static IRDataSource convertDataSource(const DataSource& dataSource);
    static IRBinding convertDataBinding(const DataBinding& binding);
    static IREffects convertStyle(const UIComponent& component);
};

class IRToProjectAdapter {
public:
    static std::unique_ptr<Project> convert(const IRProject& ir, CanvasScene* scene = nullptr);
    static void populateProject(const IRProject& ir, Project* project);

private:
    static Screen* convertScreen(const IRScreen& irScreen, QObject* parent = nullptr);
    static UIComponent* convertComponent(const IRWidget& irWidget);
    static DataSource convertDataSource(const IRDataSource& irDataSource);
    static DataBinding convertDataBinding(const IRBinding& irBinding);
};

} // namespace IR
