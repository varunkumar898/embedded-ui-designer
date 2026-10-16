#include "GeminiProvider.h"
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QElapsedTimer>

namespace AI {

GeminiProvider::GeminiProvider(QObject* parent)
    : AIProvider(parent)
{
    m_currentModel = "gemini-1.5-pro";
    m_cachedModels = {
        "gemini-1.5-pro",
        "gemini-1.5-flash",
        "gemini-2.0-flash",
        "gemini-2.0-flash-thinking-exp"
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
    QString model = m_currentModel.isEmpty() ? "gemini-1.5-pro" : m_currentModel;
    if (model.startsWith("models/")) {
        model = model.mid(7);
    }
    return QString("https://generativelanguage.googleapis.com/v1beta/models/%1:%2").arg(model, action);
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

    QUrl url("https://generativelanguage.googleapis.com/v1beta/models");
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest req(url);
    QNetworkReply* reply = m_net.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(m_cachedModels, reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray modelsArr = doc.object()["models"].toArray();

        QStringList list;
        for (const auto& mVal : modelsArr) {
            QString name = mVal.toObject()["name"].toString();
            if (name.startsWith("models/")) name = name.mid(7);
            if (name.contains("gemini")) {
                list.append(name);
            }
        }
        list.sort();
        if (!list.isEmpty()) {
            m_cachedModels = list;
        }
        callback(m_cachedModels, QString());
    });
}

void GeminiProvider::sendMessage(const QList<AIMessage>& messages,
                                const QJsonArray& tools,
                                std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback(AIMessage(), AIUsage(), "Google Gemini API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url(effectiveEndpoint("generateContent"));
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

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
        parseGeminiResponse(data, respMsg, usage);

        callback(respMsg, usage, QString());
    });
}

void GeminiProvider::streamMessage(const QList<AIMessage>& messages,
                                  const QJsonArray& tools,
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
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

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

void GeminiProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    if (!isConfigured()) {
        ConnectionTestResult res;
        res.status = ConnectionStatus::NotConfigured;
        res.message = "API key is not configured.";
        callback(res);
        return;
    }

    QUrl url("https://generativelanguage.googleapis.com/v1beta/models");
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest req(url);
    QNetworkReply* reply = m_net.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        ConnectionTestResult res;

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            res.status = ConnectionStatus::Connected;
            res.message = "Successfully connected to Google Gemini.";

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QStringList models;
            for (const auto& item : doc.object()["models"].toArray()) {
                QString name = item.toObject()["name"].toString();
                if (name.startsWith("models/")) name = name.mid(7);
                if (name.contains("gemini")) {
                    models.append(name);
                }
            }
            models.sort();
            res.detectedModels = models;
            if (!models.isEmpty()) m_cachedModels = models;
        } else {
            if (httpStatus == 400 || httpStatus == 403) {
                res.status = ConnectionStatus::AuthenticationFailed;
                res.message = "Authentication Failed: Invalid Gemini API key or unauthorized project.";
            } else if (httpStatus == 404) {
                res.status = ConnectionStatus::InvalidModel;
                res.message = "Invalid Model: Gemini model endpoint not found.";
            } else if (httpStatus == 429) {
                res.status = ConnectionStatus::RateLimited;
                res.message = "Rate Limited: Gemini API quota exceeded.";
            } else if (httpStatus >= 500) {
                res.status = ConnectionStatus::ProviderUnavailable;
                res.message = QString("Provider Unavailable: Google Gemini server error (HTTP %1).").arg(httpStatus);
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

void GeminiProvider::cancelRequest() {
    AIProvider::cancelRequest();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

} // namespace AI
