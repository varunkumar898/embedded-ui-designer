#include "GeminiProvider.h"
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QElapsedTimer>
#include <QTimer>

namespace AI {

const QString GeminiProvider::FallbackDefaultModel = "gemini-2.0-flash";

GeminiProvider::GeminiProvider(QObject* parent)
    : AIProvider(parent)
{
    m_currentModel = FallbackDefaultModel;
    m_cachedModels = {
        "gemini-2.0-flash",
        "gemini-1.5-flash",
        "gemini-1.5-pro"
    };
}

AIProviderCapabilities GeminiProvider::capabilities() const {
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

QString GeminiProvider::effectiveEndpoint(const QString& action) const {
    QString model = m_currentModel.isEmpty() ? FallbackDefaultModel : m_currentModel;
    if (model.startsWith("models/")) {
        model = model.mid(7);
    }
    QString base = m_baseUrl.isEmpty() ? "https://generativelanguage.googleapis.com" : m_baseUrl;
    while (base.endsWith('/')) base.chop(1);
    return QString("%1/v1beta/models/%2:%3").arg(base, model, action);
}

void GeminiProvider::setupAuthHeaders(QNetworkRequest& req) const {
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        req.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    }
}

bool GeminiProvider::isSuitableChatModel(const QJsonObject& modelObj, QString* outModelId) {
    QString name = modelObj["name"].toString();
    if (name.startsWith("models/")) name = name.mid(7);
    QString nameLower = name.toLower();

    if (!nameLower.contains("gemini")) {
        return false;
    }

    // Check supported generation methods (must support generateContent or streamGenerateContent)
    QJsonArray methods = modelObj["supportedGenerationMethods"].toArray();
    bool supportsGenerate = false;
    for (const auto& m : methods) {
        QString mStr = m.toString();
        if (mStr == "generateContent" || mStr == "streamGenerateContent") {
            supportsGenerate = true;
            break;
        }
    }
    if (!supportsGenerate) {
        return false;
    }

    // Exclude specialized / non-chat models
    if (nameLower.contains("computer-use") ||
        nameLower.contains("embedding") ||
        nameLower.contains("embed") ||
        nameLower.contains("imagen") ||
        nameLower.contains("image") ||
        nameLower.contains("audio") ||
        nameLower.contains("live") ||
        nameLower.contains("tts") ||
        nameLower.contains("aqa")) {
        return false;
    }

    if (outModelId) {
        *outModelId = name;
    }
    return true;
}

QString GeminiProvider::selectBestChatModel(const QStringList& suitableModels) {
    if (suitableModels.isEmpty()) return FallbackDefaultModel;
    if (suitableModels.contains("gemini-2.0-flash")) return "gemini-2.0-flash";
    if (suitableModels.contains("gemini-1.5-flash")) return "gemini-1.5-flash";
    if (suitableModels.contains("gemini-1.5-pro")) return "gemini-1.5-pro";
    if (suitableModels.contains("gemini-2.5-flash")) return "gemini-2.5-flash";
    return suitableModels.first();
}

void GeminiProvider::formatContentsForGemini(const QList<AIMessage>& messages, QJsonObject& outSystemInstruction, QJsonArray& outContents) {
    outSystemInstruction = QJsonObject();
    outContents = QJsonArray();

    for (const auto& msg : messages) {
        if (msg.role() == AIRole::System) {
            QJsonArray sysParts;
            QJsonObject part;
            part["text"] = msg.textContent();
            sysParts.append(part);
            outSystemInstruction["parts"] = sysParts;
            continue;
        }

        QJsonObject cObj;
        QString roleStr = (msg.role() == AIRole::Assistant) ? "model" : "user";
        cObj["role"] = roleStr;

        QJsonArray parts;

        // Tool Results in Gemini
        if (msg.role() == AIRole::Tool || !msg.toolResults().isEmpty()) {
            cObj["role"] = "user";
            for (const auto& tr : msg.toolResults()) {
                QJsonObject fnRespPart;
                QJsonObject fnResp;
                fnResp["name"] = tr.id; // or tool name
                QJsonObject respContent;
                if (!tr.error.isEmpty()) {
                    respContent["error"] = tr.error;
                } else {
                    respContent["output"] = tr.result;
                }
                fnResp["response"] = respContent;
                fnRespPart["functionResponse"] = fnResp;
                parts.append(fnRespPart);
            }
        }

        for (const auto& c : msg.contents()) {
            if (c.type == "text" && !c.text.isEmpty()) {
                QJsonObject part;
                part["text"] = c.text;
                parts.append(part);
            } else if (c.type == "image") {
                QJsonObject part;
                QJsonObject inlineData;
                inlineData["mimeType"] = c.imageMimeType;
                inlineData["data"] = QString::fromLatin1(c.imageData.toBase64());
                part["inlineData"] = inlineData;
                parts.append(part);
            }
        }

        if (msg.role() == AIRole::Assistant) {
            for (const auto& tc : msg.toolCalls()) {
                QJsonObject part;
                QJsonObject fnCall;
                fnCall["name"] = tc.name;
                fnCall["args"] = tc.arguments;
                part["functionCall"] = fnCall;
                parts.append(part);
            }
        }

        if (!parts.isEmpty()) {
            cObj["parts"] = parts;
            outContents.append(cObj);
        }
    }
}

QJsonArray GeminiProvider::formatToolsForGemini(const QJsonArray& universalTools) {
    QJsonArray fnDecls;
    for (const auto& val : universalTools) {
        QJsonObject t = val.toObject();
        QJsonObject fn = t.contains("function") ? t["function"].toObject() : t;

        QJsonObject decl;
        decl["name"] = fn["name"].toString();
        decl["description"] = fn["description"].toString();
        decl["parameters"] = fn["parameters"].toObject();
        fnDecls.append(decl);
    }

    QJsonArray tools;
    if (!fnDecls.isEmpty()) {
        QJsonObject toolWrap;
        toolWrap["functionDeclarations"] = fnDecls;
        tools.append(toolWrap);
    }
    return tools;
}

QJsonObject GeminiProvider::parseGeminiResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (root.contains("usageMetadata")) {
        QJsonObject uObj = root["usageMetadata"].toObject();
        outUsage.promptTokens = uObj["promptTokenCount"].toInt();
        outUsage.completionTokens = uObj["candidatesTokenCount"].toInt();
        outUsage.totalTokens = uObj["totalTokenCount"].toInt();
    }

    outMsg.setRole(AIRole::Assistant);

    QJsonArray candidates = root["candidates"].toArray();
    if (!candidates.isEmpty()) {
        QJsonObject cand = candidates.first().toObject();
        QJsonObject content = cand["content"].toObject();
        QJsonArray parts = content["parts"].toArray();

        for (const auto& pVal : parts) {
            QJsonObject pObj = pVal.toObject();
            if (pObj.contains("text")) {
                AIMessageContent mc;
                mc.type = "text";
                mc.text = pObj["text"].toString();
                outMsg.addContent(mc);
            } else if (pObj.contains("functionCall")) {
                QJsonObject fc = pObj["functionCall"].toObject();
                ToolCall tc;
                tc.name = fc["name"].toString();
                tc.id = QString("call_%1").arg(tc.name);
                tc.arguments = fc["args"].toObject();
                outMsg.addToolCall(tc);
            }
        }
    }

    return root;
}

void GeminiProvider::listModels(std::function<void(const QStringList& models, const QString& error)> callback) {
    if (!isConfigured()) {
        callback(m_cachedModels, "API key not configured");
        return;
    }

    QString base = m_baseUrl.isEmpty() ? "https://generativelanguage.googleapis.com" : m_baseUrl;
    while (base.endsWith('/')) base.chop(1);
    QUrl url(QString("%1/v1beta/models").arg(base));
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
        QJsonArray modelsArr = doc.object()["models"].toArray();

        QStringList list;
        for (const auto& mVal : modelsArr) {
            QString modelId;
            if (isSuitableChatModel(mVal.toObject(), &modelId)) {
                list.append(modelId);
            }
        }
        list.sort();
        if (!list.isEmpty()) {
            m_cachedModels = list;
            if (!m_currentModel.isEmpty() && !list.contains(m_currentModel)) {
                QString best = selectBestChatModel(list);
                QString warn = QString("Model not available; choose another (model '%1' is not available for chat; reset to '%2').")
                    .arg(m_currentModel, best);
                m_currentModel = best;
                callback(m_cachedModels, warn);
                return;
            }
        }
        callback(m_cachedModels, QString());
    });
}

void GeminiProvider::sendMessage(const QList<AIMessage>& messages,
                                const QJsonArray& tools,
                                std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    sendMessageInternal(messages, tools, 0, callback);
}

void GeminiProvider::sendMessageInternal(const QList<AIMessage>& messages,
                                        const QJsonArray& tools,
                                        int retryCount,
                                        std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback(AIMessage(), AIUsage(), "Google Gemini API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url(effectiveEndpoint("generateContent"));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QJsonObject sysInstr;
    QJsonArray contents;
    formatContentsForGemini(messages, sysInstr, contents);

    QJsonObject payload;
    if (!sysInstr.isEmpty()) {
        payload["systemInstruction"] = sysInstr;
    }
    payload["contents"] = contents;

    if (!tools.isEmpty()) {
        payload["tools"] = formatToolsForGemini(tools);
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
            // Bounded automatic retry on 429
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
        parseGeminiResponse(data, respMsg, usage);

        callback(respMsg, usage, QString());
    });
}

void GeminiProvider::streamMessage(const QList<AIMessage>& messages,
                                  const QJsonArray& tools,
                                  std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    streamMessageInternal(messages, tools, 0, callback);
}

void GeminiProvider::streamMessageInternal(const QList<AIMessage>& messages,
                                          const QJsonArray& tools,
                                          int retryCount,
                                          std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback("", {}, true, AIUsage(), "Google Gemini API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url(effectiveEndpoint("streamGenerateContent"));
    QUrlQuery query;
    query.addQueryItem("alt", "sse");
    url.setQuery(query);

    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QJsonObject sysInstr;
    QJsonArray contents;
    formatContentsForGemini(messages, sysInstr, contents);

    QJsonObject payload;
    if (!sysInstr.isEmpty()) {
        payload["systemInstruction"] = sysInstr;
    }
    payload["contents"] = contents;

    if (!tools.isEmpty()) {
        payload["tools"] = formatToolsForGemini(tools);
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
                if (doc.isObject()) {
                    QJsonObject root = doc.object();
                    if (root.contains("error")) {
                        QString errText = root["error"].toObject()["message"].toString();
                        if (errText.isEmpty()) errText = "Mid-stream error in Gemini response";
                        callback("", {}, true, *usage, redactSecrets(errText, m_apiKey));
                        return;
                    }
                }

                AIMessage chunkMsg;
                AIUsage chunkUsage;
                parseGeminiResponse(jsonPart, chunkMsg, chunkUsage);

                if (chunkUsage.totalTokens > 0) {
                    *usage = chunkUsage;
                }

                QString delta = chunkMsg.textContent();
                QList<ToolCall> calls = chunkMsg.toolCalls();
                if (!delta.isEmpty() || !calls.isEmpty()) {
                    callback(delta, calls, false, *usage, QString());
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
            // Bounded automatic retry on 429
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

void GeminiProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    if (!isConfigured()) {
        ConnectionTestResult res;
        res.status = ConnectionStatus::NotConfigured;
        res.message = "API key is not configured.";
        callback(res);
        return;
    }

    QString base = m_baseUrl.isEmpty() ? "https://generativelanguage.googleapis.com" : m_baseUrl;
    while (base.endsWith('/')) base.chop(1);
    QUrl url(QString("%1/v1beta/models").arg(base));
    QNetworkRequest req(url);
    setupAuthHeaders(req);

    QNetworkReply* reply = m_net.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, url]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            ConnectionTestResult res;
            res.status = ConnectionStatus::Connected;
            res.message = "Successfully connected to Google Gemini.";

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QStringList models;
            for (const auto& item : doc.object()["models"].toArray()) {
                QString modelId;
                if (isSuitableChatModel(item.toObject(), &modelId)) {
                    models.append(modelId);
                }
            }
            models.sort();
            res.detectedModels = models;
            if (!models.isEmpty()) {
                m_cachedModels = models;
                if (!m_currentModel.isEmpty() && !models.contains(m_currentModel)) {
                    QString best = selectBestChatModel(models);
                    res.status = ConnectionStatus::InvalidModel;
                    res.message = QString("Model not available; choose another (model '%1' is not available for chat; reset to '%2').")
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

void GeminiProvider::cancelRequest() {
    AIProvider::cancelRequest();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

} // namespace AI
