#include "AIProvider.h"
#include <QJsonDocument>

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

} // namespace AI
