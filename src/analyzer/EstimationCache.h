#pragma once

#include <QString>
#include <QMap>
#include <QDateTime>
#include "ResourceAnalysis.h"
#include "EstimationFormulas.h"

class EstimationCache {
public:
    EstimationCache() = default;

    void clear() {
        m_componentCache.clear();
        m_projectAnalysisCache.clear();
    }

    bool hasComponentEstimate(const QString& compType) const {
        return m_componentCache.contains(compType);
    }

    EstimationFormulas::ComponentFootprint getComponentEstimate(const QString& compType) const {
        return m_componentCache.value(compType);
    }

    void putComponentEstimate(const QString& compType, const EstimationFormulas::ComponentFootprint& fp) {
        m_componentCache[compType] = fp;
    }

    bool hasProjectAnalysis(const QString& key) const {
        return m_projectAnalysisCache.contains(key);
    }

    ResourceAnalysis getProjectAnalysis(const QString& key) const {
        return m_projectAnalysisCache.value(key);
    }

    void putProjectAnalysis(const QString& key, const ResourceAnalysis& analysis) {
        m_projectAnalysisCache[key] = analysis;
    }

private:
    QMap<QString, EstimationFormulas::ComponentFootprint> m_componentCache;
    QMap<QString, ResourceAnalysis> m_projectAnalysisCache;
};
