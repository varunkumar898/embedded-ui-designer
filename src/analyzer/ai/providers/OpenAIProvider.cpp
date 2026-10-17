#include "OpenAIProvider.h"
#include <QJsonDocument>
#include <QUrl>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QTimer>

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
        return normalizeBaseUrl(m_baseUrl, false);
    }
    return "https://api.openai.com/v1";
}

bool OpenAIProvider::isSuitableModel(const QString& modelId) const {
    QString id = modelId.toLower();
    if (id.contains("embed") || id.contains("moderation") || id.contains("tts") || id.contains("whisper") || id.contains("dall-e")) {
        return false;
    }
    return id.startsWith("gpt-") || id.startsWith("o1") || id.startsWith("o3") || id.contains("chat");
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, url]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            auto classified = classifyReplyError(httpStatus, reply->error(), reply->errorString(), url.toString(), displayName());
            callback(m_cachedModels, classified.message);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray dataArr = doc.object()["data"].toArray();

        QStringList models;
        for (const auto& item : dataArr) {
            QString id = item.toObject()["id"].toString();
            if (isSuitableModel(id)) {
                models.append(id);
            }
        }
        models.sort();
        if (!models.isEmpty()) {
            m_cachedModels = models;
            if (!m_currentModel.isEmpty() && !models.contains(m_currentModel)) {
                QString best = models.first();
                QString warn = QString("Model not available; choose another (model '%1' replaced with '%2')")
                    .arg(m_currentModel, best);
                m_currentModel = best;
                callback(m_cachedModels, warn);
                return;
            }
        }
        callback(m_cachedModels, QString());
    });
}

void OpenAIProvider::sendMessage(const QList<AIMessage>& messages,
                                const QJsonArray& tools,
                                std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    sendMessageInternal(messages, tools, 0, callback);
}

void OpenAIProvider::sendMessageInternal(const QList<AIMessage>& messages,
                                        const QJsonArray& tools,
                                        int retryCount,
                                        std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback(AIMessage(), AIUsage(), QString("%1 API key not configured").arg(displayName()));
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, messages, tools, retryCount, callback, url]() {
        reply->deleteLater();
        if (m_isCancelled) {
            callback(AIMessage(), AIUsage(), "Request cancelled by user");
            return;
        }

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            if (httpStatus == 429 && retryCount < 2) {
                int delayMs = 500 * (retryCount + 1);
                QByteArray retryAfter = reply->rawHeader("Retry-After");
                if (!retryAfter.isEmpty()) {
                    int secs = retryAfter.toInt();
                    if (secs > 0) delayMs = std::min(secs * 1000, 3000);
                }
                QTimer::singleShot(delayMs, this, [this, messages, tools, retryCount, callback]() {
                    if (!m_isCancelled) {
                        sendMessageInternal(messages, tools, retryCount + 1, callback);
                    }
                });
                return;
            }

            QByteArray errData = reply->readAll();
            QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            QString msg = errDoc.object()["error"].toObject()["message"].toString();
            if (msg.isEmpty()) {
                auto classified = classifyReplyError(httpStatus, reply->error(), reply->errorString(), url.toString(), displayName());
                msg = classified.message;
            } else {
                msg = redactSecrets(msg, m_apiKey);
            }
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
    streamMessageInternal(messages, tools, 0, callback);
}

void OpenAIProvider::streamMessageInternal(const QList<AIMessage>& messages,
                                          const QJsonArray& tools,
                                          int retryCount,
                                          std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback("", {}, true, AIUsage(), QString("%1 API key not configured").arg(displayName()));
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
                if (obj.contains("error")) {
                    QString errText = obj["error"].toObject()["message"].toString();
                    if (errText.isEmpty()) errText = "Mid-stream error in response";
                    callback("", {}, true, *usage, redactSecrets(errText, m_apiKey));
                    return;
                }

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

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, usage, messages, tools, retryCount, callback, url]() {
        reply->deleteLater();
        if (m_isCancelled) {
            callback("", {}, true, *usage, "Request cancelled by user");
            return;
        }

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            if (httpStatus == 429 && retryCount < 2) {
                int delayMs = 500 * (retryCount + 1);
                QByteArray retryAfter = reply->rawHeader("Retry-After");
                if (!retryAfter.isEmpty()) {
                    int secs = retryAfter.toInt();
                    if (secs > 0) delayMs = std::min(secs * 1000, 3000);
                }
                QTimer::singleShot(delayMs, this, [this, messages, tools, retryCount, callback]() {
                    if (!m_isCancelled) {
                        streamMessageInternal(messages, tools, retryCount + 1, callback);
                    }
                });
                return;
            }

            auto classified = classifyReplyError(httpStatus, reply->error(), reply->errorString(), url.toString(), displayName());
            callback("", {}, true, *usage, classified.message);
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, url]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            ConnectionTestResult res;
            res.status = ConnectionStatus::Connected;
            res.message = QString("Successfully connected to %1.").arg(displayName());

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
        } else {
            auto res = classifyReplyError(httpStatus, reply->error(), reply->errorString(), url.toString(), displayName());
            callback(res);
        }
    });
}

void OpenAIProvider::cancelRequest() {
    AIProvider::cancelRequest();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

} // namespace AI
