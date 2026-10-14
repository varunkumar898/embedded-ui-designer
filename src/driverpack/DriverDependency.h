#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>

namespace driverpack {

class DriverDependencyResolver {
public:
    DriverDependencyResolver();
    
    // Register dependency rule: module depends on list of prerequisites
    void registerDependency(const std::string& module, const std::vector<std::string>& dependencies);
    
    // Resolve full transitive closure of dependencies for a requested set of features,
    // filtered to ensure that the target device actually supports them.
    std::set<std::string> resolveDependencies(
        const std::set<std::string>& requestedFeatures,
        const std::set<std::string>& deviceSupportedPeripherals,
        std::vector<std::string>& missingPrerequisites
    ) const;

    // Get direct dependencies
    std::vector<std::string> getDirectDependencies(const std::string& module) const;

private:
    std::map<std::string, std::vector<std::string>> dependencyGraph;
};

} // namespace driverpack
