#include "AnthropicProvider.h"
#include <QJsonDocument>
#include <QUrl>
#include <QElapsedTimer>

namespace AI {

AnthropicProvider::AnthropicProvider(QObject* parent)
    : AIProvider(parent)
{
    m_currentModel = "claude-3-5-sonnet-20241022";
    m_cachedModels = {
        "claude-3-5-sonnet-20241022",
        "claude-3-5-haiku-20241022",
        "claude-3-opus-20240229",
        "claude-3-sonnet-20240229",
        "claude-3-haiku-20240307"
    };
}

AIProviderCapabilities AnthropicProvider::capabilities() const {
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

void AnthropicProvider::setupHeaders(QNetworkRequest& req) const {
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", m_apiKey.toUtf8());
    req.setRawHeader("anthropic-version", "2023-06-01");
}

void AnthropicProvider::formatMessagesForAnthropic(const QList<AIMessage>& messages, QString& outSystemPrompt, QJsonArray& outMessages) {
    outSystemPrompt.clear();
    outMessages = QJsonArray();

    for (const auto& msg : messages) {
        if (msg.role() == AIRole::System) {
            if (!outSystemPrompt.isEmpty()) outSystemPrompt += "\n\n";
            outSystemPrompt += msg.textContent();
            continue;
        }

        QJsonObject mObj;
        QString roleStr = (msg.role() == AIRole::Assistant) ? "assistant" : "user";
        mObj["role"] = roleStr;

        QJsonArray contents;

        // Add tool results if role is Tool or has tool results
        if (msg.role() == AIRole::Tool || !msg.toolResults().isEmpty()) {
            mObj["role"] = "user";
            for (const auto& tr : msg.toolResults()) {
                QJsonObject trObj;
                trObj["type"] = "tool_result";
                trObj["tool_use_id"] = tr.id;
                trObj["content"] = tr.error.isEmpty() ? tr.result : QString("Error: %1").arg(tr.error);
                if (!tr.error.isEmpty()) trObj["is_error"] = true;
                contents.append(trObj);
            }
        }

        for (const auto& c : msg.contents()) {
            if (c.type == "text" && !c.text.isEmpty()) {
                QJsonObject tObj;
                tObj["type"] = "text";
                tObj["text"] = c.text;
                contents.append(tObj);
            } else if (c.type == "image") {
                QJsonObject iObj;
                iObj["type"] = "image";
                QJsonObject srcObj;
                srcObj["type"] = "base64";
                srcObj["media_type"] = c.imageMimeType;
                srcObj["data"] = QString::fromLatin1(c.imageData.toBase64());
                iObj["source"] = srcObj;
                contents.append(iObj);
            }
        }

        // Add tool calls if this is an assistant message
        if (msg.role() == AIRole::Assistant) {
            for (const auto& tc : msg.toolCalls()) {
                QJsonObject tuObj;
                tuObj["type"] = "tool_use";
                tuObj["id"] = tc.id;
                tuObj["name"] = tc.name;
                tuObj["input"] = tc.arguments;
                contents.append(tuObj);
            }
        }

        mObj["content"] = contents;
        outMessages.append(mObj);
    }
}

QJsonArray AnthropicProvider::formatToolsForAnthropic(const QJsonArray& universalTools) {
    QJsonArray tools;
    for (const auto& val : universalTools) {
        QJsonObject t = val.toObject();
        // If wrapped in {"type": "function", "function": {...}}
        QJsonObject fn = t.contains("function") ? t["function"].toObject() : t;

        QJsonObject aTool;
        aTool["name"] = fn["name"].toString();
        aTool["description"] = fn["description"].toString();
        aTool["input_schema"] = fn["parameters"].toObject();
        tools.append(aTool);
    }
    return tools;
}

QJsonObject AnthropicProvider::parseAnthropicResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (root.contains("usage")) {
        QJsonObject uObj = root["usage"].toObject();
        outUsage.promptTokens = uObj["input_tokens"].toInt();
        outUsage.completionTokens = uObj["output_tokens"].toInt();
        outUsage.totalTokens = outUsage.promptTokens + outUsage.completionTokens;
    }

    outMsg.setRole(AIRole::Assistant);

    QJsonArray contentArr = root["content"].toArray();
    for (const auto& cVal : contentArr) {
        QJsonObject cObj = cVal.toObject();
        QString type = cObj["type"].toString();
        if (type == "text") {
            AIMessageContent mc;
            mc.type = "text";
            mc.text = cObj["text"].toString();
            outMsg.addContent(mc);
        } else if (type == "tool_use") {
            ToolCall tc;
            tc.id = cObj["id"].toString();
            tc.name = cObj["name"].toString();
            tc.arguments = cObj["input"].toObject();
            outMsg.addToolCall(tc);
        }
    }

    return root;
}

void AnthropicProvider::listModels(std::function<void(const QStringList& models, const QString& error)> callback) {
    // Anthropic doesn't have a public GET /models endpoint in older specifications,
    // but newer /v1/models exists in beta. Fallback gracefully to curated models.
    callback(m_cachedModels, QString());
}

void AnthropicProvider::sendMessage(const QList<AIMessage>& messages,
                                   const QJsonArray& tools,
                                   std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback(AIMessage(), AIUsage(), "Anthropic API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(url);
    setupHeaders(req);

    QString systemPrompt;
    QJsonArray msgs;
    formatMessagesForAnthropic(messages, systemPrompt, msgs);

    QJsonObject payload;
    payload["model"] = m_currentModel.isEmpty() ? "claude-3-5-sonnet-20241022" : m_currentModel;
    payload["max_tokens"] = 4096;
    if (!systemPrompt.isEmpty()) payload["system"] = systemPrompt;
    payload["messages"] = msgs;

    if (!tools.isEmpty()) {
        payload["tools"] = formatToolsForAnthropic(tools);
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
        parseAnthropicResponse(data, respMsg, usage);

        callback(respMsg, usage, QString());
    });
}

void AnthropicProvider::streamMessage(const QList<AIMessage>& messages,
                                     const QJsonArray& tools,
                                     std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    if (!isConfigured()) {
        callback("", {}, true, AIUsage(), "Anthropic API key not configured");
        return;
    }

    m_isCancelled = false;

    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(url);
    setupHeaders(req);

    QString systemPrompt;
    QJsonArray msgs;
    formatMessagesForAnthropic(messages, systemPrompt, msgs);

    QJsonObject payload;
    payload["model"] = m_currentModel.isEmpty() ? "claude-3-5-sonnet-20241022" : m_currentModel;
    payload["max_tokens"] = 4096;
    payload["stream"] = true;
    if (!systemPrompt.isEmpty()) payload["system"] = systemPrompt;
    payload["messages"] = msgs;

    if (!tools.isEmpty()) {
        payload["tools"] = formatToolsForAnthropic(tools);
    }

    QByteArray body = QJsonDocument(payload).toJson();
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    auto usage = std::make_shared<AIUsage>();
    auto currentToolCall = std::make_shared<ToolCall>();
    auto currentToolJson = std::make_shared<QString>();
    auto accumulatedToolCalls = std::make_shared<QList<ToolCall>>();

    QNetworkReply* reply = m_net.post(req, body);
    m_activeReply = reply;

    auto buffer = std::make_shared<QByteArray>();

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, timer, usage, buffer, currentToolCall, currentToolJson, accumulatedToolCalls, callback]() {
        buffer->append(reply->readAll());
        usage->latencyMs = timer->elapsed();

        while (true) {
            int lineEnd = buffer->indexOf('\n');
            if (lineEnd == -1) break;

            QByteArray line = buffer->left(lineEnd).trimmed();
            buffer->remove(0, lineEnd + 1);

            if (line.startsWith("data: ")) {
                QByteArray jsonPart = line.mid(6).trimmed();
                QJsonDocument doc = QJsonDocument::fromJson(jsonPart);
                if (!doc.isObject()) continue;

                QJsonObject obj = doc.object();
                QString type = obj["type"].toString();

                if (type == "content_block_start") {
                    QJsonObject cb = obj["content_block"].toObject();
                    if (cb["type"].toString() == "tool_use") {
                        currentToolCall->id = cb["id"].toString();
                        currentToolCall->name = cb["name"].toString();
                        *currentToolJson = "";
                    }
                } else if (type == "content_block_delta") {
                    QJsonObject delta = obj["delta"].toObject();
                    QString dType = delta["type"].toString();
                    if (dType == "text_delta") {
                        QString text = delta["text"].toString();
                        callback(text, {}, false, *usage, QString());
                    } else if (dType == "input_json_delta") {
                        currentToolJson->append(delta["partial_json"].toString());
                    }
                } else if (type == "content_block_stop") {
                    if (!currentToolCall->id.isEmpty()) {
                        currentToolCall->arguments = QJsonDocument::fromJson(currentToolJson->toUtf8()).object();
                        accumulatedToolCalls->append(*currentToolCall);
                        currentToolCall->id = "";
                        currentToolCall->name = "";
                        *currentToolJson = "";
                    }
                } else if (type == "message_delta") {
                    if (obj.contains("usage")) {
                        usage->completionTokens = obj["usage"].toObject()["output_tokens"].toInt();
                        usage->totalTokens = usage->promptTokens + usage->completionTokens;
                    }
                } else if (type == "message_stop") {
                    callback("", *accumulatedToolCalls, true, *usage, QString());
                    return;
                }
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, usage, accumulatedToolCalls, callback]() {
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
        callback("", *accumulatedToolCalls, true, *usage, QString());
    });
}

void AnthropicProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    if (!isConfigured()) {
        ConnectionTestResult res;
        res.status = ConnectionStatus::NotConfigured;
        res.message = "API key is not configured.";
        callback(res);
        return;
    }

    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest req(url);
    setupHeaders(req);

    QJsonObject payload;
    payload["model"] = m_currentModel.isEmpty() ? "claude-3-5-haiku-20241022" : m_currentModel;
    payload["max_tokens"] = 1;

    QJsonArray msgs;
    QJsonObject m;
    m["role"] = "user";
    m["content"] = "ping";
    msgs.append(m);
    payload["messages"] = msgs;

    QByteArray body = QJsonDocument(payload).toJson();
    QNetworkReply* reply = m_net.post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        ConnectionTestResult res;

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            res.status = ConnectionStatus::Connected;
            res.message = "Successfully connected to Anthropic Claude.";
            res.detectedModels = m_cachedModels;
        } else {
            if (httpStatus == 401) {
                res.status = ConnectionStatus::AuthenticationFailed;
                res.message = "Authentication Failed: Invalid Anthropic API key.";
            } else if (httpStatus == 404) {
                res.status = ConnectionStatus::InvalidModel;
                res.message = QString("Invalid Model: Specified Anthropic model '%1' not recognized.").arg(m_currentModel);
            } else if (httpStatus == 429) {
                res.status = ConnectionStatus::RateLimited;
                res.message = "Rate Limited: Anthropic usage limit exceeded.";
            } else if (httpStatus >= 500) {
                res.status = ConnectionStatus::ProviderUnavailable;
                res.message = QString("Provider Unavailable: Anthropic service error (HTTP %1).").arg(httpStatus);
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

void AnthropicProvider::cancelRequest() {
    AIProvider::cancelRequest();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

} // namespace AI
