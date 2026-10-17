#include "AIProvider.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QNetworkReply>

namespace AI {

QString roleToString(AIRole role) {
    switch (role) {
        case AIRole::System: return "system";
        case AIRole::User: return "user";
        case AIRole::Assistant: return "assistant";
        case AIRole::Tool: return "tool";
        default: return "user";
    }
}

AIRole stringToRole(const QString& str) {
    QString lower = str.toLower();
    if (lower == "system") return AIRole::System;
    if (lower == "assistant") return AIRole::Assistant;
    if (lower == "tool") return AIRole::Tool;
    return AIRole::User;
}

QString connectionStatusToString(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::Connected: return "Connected";
        case ConnectionStatus::AuthenticationFailed: return "Authentication Failed";
        case ConnectionStatus::InvalidModel: return "Invalid Model";
        case ConnectionStatus::RateLimited: return "Rate Limited";
        case ConnectionStatus::NetworkError: return "Network Error";
        case ConnectionStatus::ProviderUnavailable: return "Provider Unavailable";
        case ConnectionStatus::NotConfigured: return "Not Configured";
        default: return "Unknown";
    }
}

AIMessage::AIMessage(AIRole role, const QString& textContent)
    : m_role(role)
{
    if (!textContent.isEmpty()) {
        AIMessageContent content;
        content.type = "text";
        content.text = textContent;
        m_contents.append(content);
    }
}

QString AIMessage::textContent() const {
    QString result;
    for (const auto& c : m_contents) {
        if (c.type == "text") {
            if (!result.isEmpty()) result += "\n";
            result += c.text;
        }
    }
    return result;
}

void AIMessage::setTextContent(const QString& text) {
    m_contents.clear();
    AIMessageContent content;
    content.type = "text";
    content.text = text;
    m_contents.append(content);
}

void AIMessage::addImageContent(const QByteArray& data, const QString& mimeType) {
    AIMessageContent content;
    content.type = "image";
    content.imageData = data;
    content.imageMimeType = mimeType;
    m_contents.append(content);
}

QJsonObject AIMessage::toJson() const {
    QJsonObject obj;
    obj["role"] = roleToString(m_role);

    QJsonArray contentArr;
    for (const auto& c : m_contents) {
        QJsonObject cObj;
        cObj["type"] = c.type;
        if (c.type == "text") {
            cObj["text"] = c.text;
        } else if (c.type == "image") {
            cObj["mime_type"] = c.imageMimeType;
            cObj["data"] = QString::fromLatin1(c.imageData.toBase64());
        }
        contentArr.append(cObj);
    }
    obj["contents"] = contentArr;

    if (!m_toolCalls.isEmpty()) {
        QJsonArray callsArr;
        for (const auto& call : m_toolCalls) {
            QJsonObject callObj;
            callObj["id"] = call.id;
            callObj["name"] = call.name;
            callObj["arguments"] = call.arguments;
            callsArr.append(callObj);
        }
        obj["tool_calls"] = callsArr;
    }

    if (!m_toolResults.isEmpty()) {
        QJsonArray resArr;
        for (const auto& res : m_toolResults) {
            QJsonObject resObj;
            resObj["id"] = res.id;
            resObj["result"] = res.result;
            if (!res.error.isEmpty()) resObj["error"] = res.error;
            resArr.append(resObj);
        }
        obj["tool_results"] = resArr;
    }

    if (!m_metadata.isEmpty()) {
        obj["metadata"] = m_metadata;
    }

    return obj;
}

AIMessage AIMessage::fromJson(const QJsonObject& obj) {
    AIMessage msg;
    msg.setRole(stringToRole(obj["role"].toString()));

    QJsonArray contentArr = obj["contents"].toArray();
    for (const auto& cVal : contentArr) {
        QJsonObject cObj = cVal.toObject();
        AIMessageContent c;
        c.type = cObj["type"].toString();
        if (c.type == "text") {
            c.text = cObj["text"].toString();
        } else if (c.type == "image") {
            c.imageMimeType = cObj["mime_type"].toString("image/png");
            c.imageData = QByteArray::fromBase64(cObj["data"].toString().toLatin1());
        }
        msg.addContent(c);
    }

    // Fallback if content was stored as plain string
    if (contentArr.isEmpty() && obj.contains("content")) {
        msg.setTextContent(obj["content"].toString());
    }

    if (obj.contains("tool_calls")) {
        QJsonArray callsArr = obj["tool_calls"].toArray();
        for (const auto& val : callsArr) {
            QJsonObject cObj = val.toObject();
            ToolCall call;
            call.id = cObj["id"].toString();
            call.name = cObj["name"].toString();
            call.arguments = cObj["arguments"].toObject();
            msg.addToolCall(call);
        }
    }

    if (obj.contains("tool_results")) {
        QJsonArray resArr = obj["tool_results"].toArray();
        for (const auto& val : resArr) {
            QJsonObject rObj = val.toObject();
            ToolResult res;
            res.id = rObj["id"].toString();
            res.result = rObj["result"].toString();
            res.error = rObj["error"].toString();
            msg.addToolResult(res);
        }
    }

    if (obj.contains("metadata")) {
        msg.setMetadata(obj["metadata"].toObject());
    }

    return msg;
}

void AIConversation::addUsage(const AIUsage& usage) {
    m_cumulativeUsage.promptTokens += usage.promptTokens;
    m_cumulativeUsage.completionTokens += usage.completionTokens;
    m_cumulativeUsage.totalTokens += usage.totalTokens;
    m_cumulativeUsage.latencyMs += usage.latencyMs;
}

QJsonObject AIConversation::toJson() const {
    QJsonObject obj;
    obj["provider_id"] = m_providerId;
    obj["model"] = m_model;

    QJsonArray msgsArr;
    for (const auto& msg : m_messages) {
        msgsArr.append(msg.toJson());
    }
    obj["messages"] = msgsArr;

    QJsonObject usageObj;
    usageObj["prompt_tokens"] = m_cumulativeUsage.promptTokens;
    usageObj["completion_tokens"] = m_cumulativeUsage.completionTokens;
    usageObj["total_tokens"] = m_cumulativeUsage.totalTokens;
    usageObj["latency_ms"] = m_cumulativeUsage.latencyMs;
    obj["cumulative_usage"] = usageObj;

    return obj;
}

AIConversation AIConversation::fromJson(const QJsonObject& obj) {
    AIConversation conv;
    conv.setProviderId(obj["provider_id"].toString());
    conv.setModel(obj["model"].toString());

    QJsonArray msgsArr = obj["messages"].toArray();
    for (const auto& mVal : msgsArr) {
        conv.addMessage(AIMessage::fromJson(mVal.toObject()));
    }

    QJsonObject uObj = obj["cumulative_usage"].toObject();
    AIUsage usage;
    usage.promptTokens = uObj["prompt_tokens"].toInt();
    usage.completionTokens = uObj["completion_tokens"].toInt();
    usage.totalTokens = uObj["total_tokens"].toInt();
    usage.latencyMs = uObj["latency_ms"].toInt();
    conv.addUsage(usage);

    return conv;
}

AIProvider::AIProvider(QObject* parent)
    : QObject(parent)
{
}

bool AIProvider::isConfigured() const {
    return !m_apiKey.trimmed().isEmpty();
}

void AIProvider::configure(const QString& apiKey, const QString& model, const QString& baseUrl) {
    m_apiKey = apiKey.trimmed();
    if (!model.isEmpty()) m_currentModel = model.trimmed();
    if (!baseUrl.isEmpty()) m_baseUrl = baseUrl.trimmed();
    emit configurationChanged();
}

void AIProvider::cancelRequest() {
    m_isCancelled = true;
    emit requestCancelled();
}

QString AIProvider::redactSecrets(const QString& text, const QString& knownKey) {
    if (text.isEmpty()) return text;
    QString result = text;

    // 1. Redact known key if provided and not trivial (length >= 4)
    if (!knownKey.isEmpty() && knownKey.length() >= 4) {
        result.replace(knownKey, "***");
    }

    // 2. Redact query parameters in URLs (e.g. ?key=... or &key=... or apiKey=...)
    static const QRegularExpression queryRegex(
        R"(([\?&](?:key|apiKey|api_key|token|auth)=)([^&\s"'<>#]+))",
        QRegularExpression::CaseInsensitiveOption);
    result.replace(queryRegex, QStringLiteral("\\1***"));

    // 3. Redact Authorization Bearer headers or tokens
    static const QRegularExpression bearerRegex(
        R"((Bearer\s+)[A-Za-z0-9_\-\.]{6,})",
        QRegularExpression::CaseInsensitiveOption);
    result.replace(bearerRegex, QStringLiteral("\\1***"));

    // 4. Redact API key headers (e.g. x-goog-api-key, x-api-key)
    static const QRegularExpression headerRegex(
        R"(((?:x-goog-api-key|x-api-key|api-key):\s*)[^\s"'<>]+)",
        QRegularExpression::CaseInsensitiveOption);
    result.replace(headerRegex, QStringLiteral("\\1***"));

    // 5. Redact typical API key token formats (Google, OpenAI, Anthropic)
    static const QRegularExpression geminiKeyRegex(R"(AIza[0-9A-Za-z\-_]{35})");
    result.replace(geminiKeyRegex, QStringLiteral("***"));

    static const QRegularExpression openAiKeyRegex(R"(sk-(?:proj-)?[A-Za-z0-9\-_]{20,})");
    result.replace(openAiKeyRegex, QStringLiteral("***"));

    static const QRegularExpression anthropicKeyRegex(R"(sk-ant-[A-Za-z0-9\-_]{20,})");
    result.replace(anthropicKeyRegex, QStringLiteral("***"));

    return result;
}

QString AIProvider::maskedKeyDisplay(const QString& rawKey) {
    QString trimmed = rawKey.trimmed();
    if (trimmed.isEmpty()) return QString();
    if (trimmed.length() <= 4) return QStringLiteral("****");
    return QStringLiteral("••••••••••••") + trimmed.right(4);
}

ConnectionTestResult AIProvider::classifyReplyError(int httpStatus, int qtNetworkError, const QString& qtErrorString, const QString& urlStr, const QString& providerName) {
    ConnectionTestResult res;
    QString cleanUrl = redactSecrets(urlStr);

    if (qtNetworkError == QNetworkReply::ConnectionRefusedError) {
        res.status = ConnectionStatus::NetworkError;
        res.message = QString("Cannot reach server at %1. Is it running?").arg(cleanUrl);
    } else if (qtNetworkError == QNetworkReply::TimeoutError) {
        res.status = ConnectionStatus::NetworkError;
        res.message = QString("Connection to %1 timed out.").arg(cleanUrl);
    } else if (qtNetworkError == QNetworkReply::HostNotFoundError) {
        res.status = ConnectionStatus::NetworkError;
        res.message = QString("DNS lookup failed: host in %1 not found.").arg(cleanUrl);
    } else if (qtNetworkError == QNetworkReply::SslHandshakeFailedError) {
        res.status = ConnectionStatus::NetworkError;
        res.message = QString("TLS/SSL handshake failed connecting to %1.").arg(cleanUrl);
    } else if (httpStatus == 401 || httpStatus == 403) {
        res.status = ConnectionStatus::AuthenticationFailed;
        res.message = QString("Authentication Failed: API key rejected or unauthorized for %1.").arg(providerName);
    } else if (httpStatus == 404) {
        res.status = ConnectionStatus::InvalidModel;
        res.message = QString("Model or endpoint path not found at %1. (For OpenAI-compatible servers, verify the path includes '/v1').").arg(cleanUrl);
    } else if (httpStatus == 429) {
        res.status = ConnectionStatus::RateLimited;
        res.message = QString("Rate limit or quota reached for %1. Please check your plan or try a Flash-class model.").arg(providerName);
    } else if (httpStatus >= 500) {
        res.status = ConnectionStatus::ProviderUnavailable;
        res.message = QString("Provider Unavailable: %1 server error (HTTP %2).").arg(providerName).arg(httpStatus);
    } else {
        res.status = ConnectionStatus::NetworkError;
        res.message = QString("Connection failed for %1: %2 (HTTP %3)")
            .arg(providerName, redactSecrets(qtErrorString), QString::number(httpStatus));
    }
    return res;
}

QString AIProvider::normalizeBaseUrl(const QString& rawUrl, bool ensureV1) {
    QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty()) return trimmed;
    if (!trimmed.startsWith("http://", Qt::CaseInsensitive) && !trimmed.startsWith("https://", Qt::CaseInsensitive)) {
        trimmed = "http://" + trimmed;
    }
    while (trimmed.endsWith('/')) {
        trimmed.chop(1);
    }
    if (ensureV1) {
        if (!trimmed.endsWith("/v1", Qt::CaseInsensitive) && !trimmed.endsWith("/v1beta", Qt::CaseInsensitive)) {
            trimmed += "/v1";
        }
    }
    return trimmed;
}

} // namespace AI
