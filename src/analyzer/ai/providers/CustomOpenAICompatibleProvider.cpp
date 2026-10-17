#include "CustomOpenAICompatibleProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace AI {

const QString CustomOpenAICompatibleProvider::PresetLMStudio = "http://127.0.0.1:1234/v1";
const QString CustomOpenAICompatibleProvider::PresetOllama = "http://127.0.0.1:11434/v1";

CustomOpenAICompatibleProvider::CustomOpenAICompatibleProvider(QObject* parent)
    : OpenAIProvider(parent)
{
    m_baseUrl = PresetLMStudio;
    m_currentModel = "llama3";
    m_cachedModels = {"llama3", "mistral", "qwen2.5-coder", "phi3"};
}

bool CustomOpenAICompatibleProvider::isConfigured() const {
    // Local providers might not require an API key, but do require a base URL
    return !m_baseUrl.trimmed().isEmpty();
}

QString CustomOpenAICompatibleProvider::effectiveBaseUrl() const {
    QString base = m_baseUrl.trimmed().isEmpty() ? PresetLMStudio : m_baseUrl.trimmed();
    return normalizeBaseUrl(base, true);
}

bool CustomOpenAICompatibleProvider::isSuitableModel(const QString& modelId) const {
    QString id = modelId.toLower();
    if (id.contains("embed") || id.contains("bge") || id.contains("rerank")) {
        return false;
    }
    return true;
}

void CustomOpenAICompatibleProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    if (!isConfigured()) {
        ConnectionTestResult res;
        res.status = ConnectionStatus::NotConfigured;
        res.message = "Base URL is not configured.";
        callback(res);
        return;
    }

    QString currentBase = effectiveBaseUrl();
    QUrl url(QString("%1/models").arg(currentBase));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QNetworkReply* reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, currentBase, url]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            ConnectionTestResult res;
            res.status = ConnectionStatus::Connected;
            res.message = QString("Successfully connected to local server at %1.").arg(currentBase);

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QStringList models;
            for (const auto& item : doc.object()["data"].toArray()) {
                QString id = item.toObject()["id"].toString();
                if (isSuitableModel(id)) {
                    models.append(id);
                }
            }
            models.sort();
            res.detectedModels = models;
            if (!models.isEmpty()) {
                m_cachedModels = models;
                if (!m_currentModel.isEmpty() && !models.contains(m_currentModel)) {
                    QString best = models.first();
                    res.status = ConnectionStatus::InvalidModel;
                    res.message = QString("Model not available; choose another (model '%1' is not available; reset to '%2').")
                        .arg(m_currentModel, best);
                    m_currentModel = best;
                }
            }
            callback(res);
            return;
        }

        // If connection refused, try dual-host fallback (localhost <-> 127.0.0.1)
        if (reply->error() == QNetworkReply::ConnectionRefusedError) {
            QString altBase;
            if (currentBase.contains("localhost")) {
                altBase = currentBase;
                altBase.replace("localhost", "127.0.0.1");
            } else if (currentBase.contains("127.0.0.1")) {
                altBase = currentBase;
                altBase.replace("127.0.0.1", "localhost");
            }

            if (!altBase.isEmpty() && altBase != currentBase) {
                QUrl altUrl(QString("%1/models").arg(altBase));
                QNetworkRequest altReq(altUrl);
                setupAuthHeaders(altReq);

                QNetworkReply* altReply = m_net.get(altReq);
                connect(altReply, &QNetworkReply::finished, this, [this, altReply, callback, altBase, currentBase]() {
                    altReply->deleteLater();
                    int altStatus = altReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

                    if (altReply->error() == QNetworkReply::NoError && altStatus == 200) {
                        m_baseUrl = altBase;
                        ConnectionTestResult res;
                        res.status = ConnectionStatus::Connected;
                        res.message = QString("Successfully connected to local server at %1 (automatically switched host).").arg(altBase);

                        QByteArray data = altReply->readAll();
                        QJsonDocument doc = QJsonDocument::fromJson(data);
                        QStringList models;
                        for (const auto& item : doc.object()["data"].toArray()) {
                            QString id = item.toObject()["id"].toString();
                            if (isSuitableModel(id)) {
                                models.append(id);
                            }
                        }
                        models.sort();
                        res.detectedModels = models;
                        if (!models.isEmpty()) {
                            m_cachedModels = models;
                            if (!m_currentModel.isEmpty() && !models.contains(m_currentModel)) {
                                m_currentModel = models.first();
                            }
                        }
                        callback(res);
                    } else {
                        // Both failed with connection refused
                        ConnectionTestResult res;
                        res.status = ConnectionStatus::NetworkError;
                        res.message = QString("Cannot reach server at %1. Is it running?").arg(currentBase);
                        callback(res);
                    }
                });
                return;
            }
        }

        auto res = classifyReplyError(httpStatus, reply->error(), reply->errorString(), url.toString(), displayName());
        callback(res);
    });
}

} // namespace AI
