#include "OpenAIProvider.h"
#include <QJsonDocument>
#include <QUrl>
#include <QElapsedTimer>
#include <QRegularExpression>

namespace AI {

OpenAIProvider::OpenAIProvider(QObject* parent)
    : AIProvider(parent)
{
    m_currentModel = "gpt-4o";
    m_cachedModels = {"gpt-4o", "gpt-4o-mini", "o1", "o3-mini"};
}

AIProviderCapabilities OpenAIProvider::capabilities() const {
    AIProviderCapabilities caps;
    caps.textInput = true;
    caps.textOutput = true;
    caps.imageInput = true;
    caps.imageOutput = false;
    caps.streaming = true;
    caps.toolCalling = true;
    caps.structuredOutput = true;
    caps.longContext = true;
    caps.conversationState = true;
    return caps;
}

QString OpenAIProvider::effectiveBaseUrl() const {
    if (!m_baseUrl.isEmpty()) {
        return m_baseUrl.endsWith('/') ? m_baseUrl.left(m_baseUrl.length() - 1) : m_baseUrl;
    }
    return "https://api.openai.com/v1";
}

void OpenAIProvider::setupAuthHeaders(QNetworkRequest& req) const {
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        req.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    }
}

QJsonArray OpenAIProvider::formatMessagesForOpenAI(const QList<AIMessage>& messages) {
    QJsonArray array;
    for (const auto& msg : messages) {
        QJsonObject mObj;
        mObj["role"] = roleToString(msg.role());

        if (msg.role() == AIRole::Tool) {
            // For tool response in OpenAI
            if (!msg.toolResults().isEmpty()) {
                const auto& tr = msg.toolResults().first();
                mObj["tool_call_id"] = tr.id;
                mObj["content"] = tr.error.isEmpty() ? tr.result : QString("Error: %1").arg(tr.error);
            } else {
                mObj["content"] = msg.textContent();
            }
            array.append(mObj);
            continue;
        }

        // Check if message has images
        bool hasImages = false;
        for (const auto& c : msg.contents()) {
            if (c.type == "image") {
                hasImages = true;
                break;
            }
        }

        if (hasImages) {
            QJsonArray parts;
            for (const auto& c : msg.contents()) {
                if (c.type == "text") {
                    QJsonObject tPart;
                    tPart["type"] = "text";
                    tPart["text"] = c.text;
                    parts.append(tPart);
                } else if (c.type == "image") {
                    QJsonObject iPart;
                    iPart["type"] = "image_url";
                    QJsonObject urlObj;
                    urlObj["url"] = QString("data:%1;base64,%2").arg(c.imageMimeType, QString::fromLatin1(c.imageData.toBase64()));
                    iPart["image_url"] = urlObj;
                    parts.append(iPart);
                }
            }
            mObj["content"] = parts;
        } else {
            mObj["content"] = msg.textContent();
        }

        if (!msg.toolCalls().isEmpty()) {
            QJsonArray callsArr;
            for (const auto& tc : msg.toolCalls()) {
                QJsonObject tcObj;
                tcObj["id"] = tc.id;
                tcObj["type"] = "function";
                QJsonObject fnObj;
                fnObj["name"] = tc.name;
                fnObj["arguments"] = QString::fromUtf8(QJsonDocument(tc.arguments).toJson(QJsonDocument::Compact));
                tcObj["function"] = fnObj;
                callsArr.append(tcObj);
            }
            mObj["tool_calls"] = callsArr;
        }

        array.append(mObj);
    }
    return array;
}

QJsonObject OpenAIProvider::parseOpenAIResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (root.contains("usage")) {
        QJsonObject uObj = root["usage"].toObject();
        outUsage.promptTokens = uObj["prompt_tokens"].toInt();
        outUsage.completionTokens = uObj["completion_tokens"].toInt();
        outUsage.totalTokens = uObj["total_tokens"].toInt();
    }

    if (root.contains("choices")) {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices.first().toObject();
            QJsonObject msgObj = firstChoice["message"].toObject();
            outMsg.setRole(stringToRole(msgObj["role"].toString()));
            outMsg.setTextContent(msgObj["content"].toString());

            if (msgObj.contains("tool_calls")) {
                QJsonArray tcArr = msgObj["tool_calls"].toArray();
                for (const auto& tcVal : tcArr) {
                    QJsonObject tcObj = tcVal.toObject();
                    ToolCall tc;
                    tc.id = tcObj["id"].toString();
                    QJsonObject fnObj = tcObj["function"].toObject();
                    tc.name = fnObj["name"].toString();
                    tc.arguments = QJsonDocument::fromJson(fnObj["arguments"].toString().toUtf8()).object();
                    outMsg.addToolCall(tc);
                }
            }
        }
    }

    return root;
}

void OpenAIProvider::listModels(std::function<void(const QStringList& models, const QString& error)> callback) {
    if (!isConfigured()) {
        callback(m_cachedModels, "API key not configured");
        return;
    }

    QUrl url(QString("%1/models").arg(effectiveBaseUrl()));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QNetworkReply* reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(m_cachedModels, reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray dataArr = doc.object()["data"].toArray();

        QStringList models;
        for (const auto& item : dataArr) {
            QString id = item.toObject()["id"].toString();
            if (id.startsWith("gpt-") || id.startsWith("o1") || id.startsWith("o3")) {
                models.append(id);
            }
        }
        models.sort();
        if (!models.isEmpty()) {
            m_cachedModels = models;
        }
        callback(m_cachedModels, QString());
    });
}

void OpenAIProvider::sendMessage(const QList<AIMessage>& messages,
                                const QJsonArray& tools,
                                std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback(AIMessage(), AIUsage(), "OpenAI API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url(QString("%1/chat/completions").arg(effectiveBaseUrl()));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QJsonObject payload;
    payload["model"] = m_currentModel.isEmpty() ? "gpt-4o" : m_currentModel;
    payload["messages"] = formatMessagesForOpenAI(messages);

    if (!tools.isEmpty()) {
        payload["tools"] = tools;
    }

    QByteArray body = QJsonDocument(payload).toJson();

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    QNetworkReply* reply = m_net.post(req, body);
    m_activeReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, callback]() {
        reply->deleteLater();
        if (m_isCancelled) {
            callback(AIMessage(), AIUsage(), "Request cancelled by user");
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            QByteArray errData = reply->readAll();
            QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            QString msg = errDoc.object()["error"].toObject()["message"].toString();
            if (msg.isEmpty()) msg = reply->errorString();
            callback(AIMessage(), AIUsage(), msg);
            return;
        }

        QByteArray data = reply->readAll();
        AIMessage respMsg;
        AIUsage usage;
        usage.latencyMs = timer->elapsed();
        parseOpenAIResponse(data, respMsg, usage);

        callback(respMsg, usage, QString());
    });
}

void OpenAIProvider::streamMessage(const QList<AIMessage>& messages,
                                  const QJsonArray& tools,
                                  std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback("", {}, true, AIUsage(), "OpenAI API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url(QString("%1/chat/completions").arg(effectiveBaseUrl()));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QJsonObject payload;
    payload["model"] = m_currentModel.isEmpty() ? "gpt-4o" : m_currentModel;
    payload["messages"] = formatMessagesForOpenAI(messages);
    payload["stream"] = true;

    if (!tools.isEmpty()) {
        payload["tools"] = tools;
    }

    QByteArray body = QJsonDocument(payload).toJson();
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    auto usage = std::make_shared<AIUsage>();

    QNetworkReply* reply = m_net.post(req, body);
    m_activeReply = reply;

    auto buffer = std::make_shared<QByteArray>();

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, timer, usage, buffer, callback]() {
        buffer->append(reply->readAll());
        usage->latencyMs = timer->elapsed();

        while (true) {
            int lineEnd = buffer->indexOf('\n');
            if (lineEnd == -1) break;

            QByteArray line = buffer->left(lineEnd).trimmed();
            buffer->remove(0, lineEnd + 1);

            if (line.startsWith("data: ")) {
                QByteArray jsonPart = line.mid(6).trimmed();
                if (jsonPart == "[DONE]") {
                    callback("", {}, true, *usage, QString());
                    return;
                }

                QJsonDocument doc = QJsonDocument::fromJson(jsonPart);
                if (!doc.isObject()) continue;

                QJsonObject obj = doc.object();
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject delta = choices.first().toObject()["delta"].toObject();
                    QString deltaText = delta["content"].toString();
                    QList<ToolCall> toolCalls;
                    if (delta.contains("tool_calls")) {
                        for (const auto& tcVal : delta["tool_calls"].toArray()) {
                            QJsonObject tcObj = tcVal.toObject();
                            ToolCall tc;
                            tc.id = tcObj["id"].toString();
                            tc.name = tcObj["function"].toObject()["name"].toString();
                            tc.arguments = QJsonDocument::fromJson(tcObj["function"].toObject()["arguments"].toString().toUtf8()).object();
                            toolCalls.append(tc);
                        }
                    }
                    if (!deltaText.isEmpty() || !toolCalls.isEmpty()) {
                        callback(deltaText, toolCalls, false, *usage, QString());
                    }
                }
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, usage, callback]() {
        reply->deleteLater();
        if (m_isCancelled) {
            callback("", {}, true, *usage, "Request cancelled by user");
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            callback("", {}, true, *usage, reply->errorString());
            return;
        }

        usage->latencyMs = timer->elapsed();
        callback("", {}, true, *usage, QString());
    });
}

void OpenAIProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    if (!isConfigured()) {
        ConnectionTestResult res;
        res.status = ConnectionStatus::NotConfigured;
        res.message = "API key is not configured.";
        callback(res);
        return;
    }

    QUrl url(QString("%1/models").arg(effectiveBaseUrl()));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QNetworkReply* reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        ConnectionTestResult res;

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            res.status = ConnectionStatus::Connected;
            res.message = "Successfully connected to OpenAI.";

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QStringList models;
            for (const auto& item : doc.object()["data"].toArray()) {
                QString id = item.toObject()["id"].toString();
                if (id.startsWith("gpt-") || id.startsWith("o1") || id.startsWith("o3")) {
                    models.append(id);
                }
            }
            models.sort();
            res.detectedModels = models;
            if (!models.isEmpty()) m_cachedModels = models;
        } else {
            if (httpStatus == 401) {
                res.status = ConnectionStatus::AuthenticationFailed;
                res.message = "Authentication Failed: Invalid OpenAI API key.";
            } else if (httpStatus == 429) {
                res.status = ConnectionStatus::RateLimited;
                res.message = "Rate Limited: OpenAI quota or rate limit reached.";
            } else if (httpStatus >= 500) {
                res.status = ConnectionStatus::ProviderUnavailable;
                res.message = QString("Provider Unavailable: OpenAI server error (HTTP %1).").arg(httpStatus);
            } else if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::TimeoutError) {
                res.status = ConnectionStatus::NetworkError;
                res.message = QString("Network Error: %1").arg(reply->errorString());
            } else {
                res.status = ConnectionStatus::AuthenticationFailed;
                res.message = QString("Connection failed: %1 (HTTP %2)").arg(reply->errorString()).arg(httpStatus);
            }
        }

        callback(res);
    });
}

void OpenAIProvider::cancelRequest() {
    AIProvider::cancelRequest();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

} // namespace AI
