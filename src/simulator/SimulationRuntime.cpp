#include "SimulationRuntime.h"
#include "ValueVisualizationComponent.h"
#include "ProgressBarComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "LabelComponent.h"
#include "TextInputComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"
#include "HardwareManager.h"
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <QDebug>

namespace Simulator {

SimulationRuntime::SimulationRuntime(Project* project, QObject* parent)
    : QObject(parent)
    , m_project(project)
    , m_backend(std::make_shared<SimulationBackend>())
    , m_tickTimer(new QTimer(this))
{
    connect(m_tickTimer, &QTimer::timeout, this, &SimulationRuntime::onTickTimer);

    // Register simulation backend with HardwareManager
    Hardware::HardwareManager::instance().registerBackend(m_backend);

    setupInitialState();
}

SimulationRuntime::~SimulationRuntime() {
    stop();
    if (Hardware::HardwareManager::instance().activeBackend() == m_backend.get()) {
        Hardware::HardwareManager::instance().setActiveBackend("mock");
    }
}

void SimulationRuntime::setupInitialState() {
    if (!m_project) return;

    m_activeScreenId = m_project->activeScreen() ? m_project->activeScreen()->id() : QString();
    m_originalActiveScreenId = m_activeScreenId;

    // Initialize simulated data source values from project definitions
    for (const auto& ds : m_project->dataSources()) {
        QVariant val = ds.value();
        if (!val.isValid()) {
            switch (ds.dataType()) {
            case DataType::Boolean: val = false; break;
            case DataType::Integer:
            case DataType::UnsignedInteger: val = 0; break;
            case DataType::Float: val = 0.0; break;
            case DataType::String: val = QString(); break;
            default: val = 0.0; break;
            }
        }
        m_simulatedValues[ds.id()] = val;
        m_originalDataSourceValues[ds.id()] = val;
    }

    updateCalculatedDataSources();
}

void SimulationRuntime::saveProjectDesignSnapshot() {
    m_designSnapshot.clear();
    if (!m_project) return;

    for (Screen* scr : m_project->screens()) {
        if (!scr) continue;
        for (UIComponent* comp : scr->components()) {
            if (!comp) continue;
            ComponentSnapshot snap;
            snap.originalJson = comp->toJson();
            m_designSnapshot[comp->componentId()] = snap;
        }
    }
}

void SimulationRuntime::restoreProjectDesignSnapshot() {
    if (!m_project) return;

    for (Screen* scr : m_project->screens()) {
        if (!scr) continue;
        for (UIComponent* comp : scr->components()) {
            if (!comp) continue;
            if (m_designSnapshot.contains(comp->componentId())) {
                comp->fromJson(m_designSnapshot[comp->componentId()].originalJson);
            }
        }
    }

    // Restore DataSources
    for (const auto& key : m_originalDataSourceValues.keys()) {
        m_simulatedValues[key] = m_originalDataSourceValues[key];
    }

    if (!m_originalActiveScreenId.isEmpty() && m_project->findScreen(m_originalActiveScreenId)) {
        m_project->setActiveScreenById(m_originalActiveScreenId);
        m_activeScreenId = m_originalActiveScreenId;
    }
}

void SimulationRuntime::start() {
    if (m_status == SimulationStatus::Running) return;

    if (m_status == SimulationStatus::Stopped) {
        saveProjectDesignSnapshot();
        m_simulatedElapsedMs = 0;

        // Switch HardwareManager to simulation backend safely
        Hardware::HardwareManager::instance().setActiveBackend("simulation");

        // Initial propagation of all data sources to bindings
        for (auto it = m_simulatedValues.begin(); it != m_simulatedValues.end(); ++it) {
            propagateDataSourceBindings(it.key(), it.value());
        }
    }

    m_status = SimulationStatus::Running;
    int interval = static_cast<int>(50.0 / std::max(0.1, m_speedFactor));
    m_tickTimer->start(std::max(5, interval));

    emit statusChanged(m_status);
}

void SimulationRuntime::pause() {
    if (m_status != SimulationStatus::Running) return;
    m_status = SimulationStatus::Paused;
    m_tickTimer->stop();
    emit statusChanged(m_status);
}

void SimulationRuntime::resume() {
    if (m_status == SimulationStatus::Paused) {
        start();
    }
}

void SimulationRuntime::stop() {
    if (m_status == SimulationStatus::Stopped) return;

    m_tickTimer->stop();
    m_status = SimulationStatus::Stopped;

    restoreProjectDesignSnapshot();

    emit statusChanged(m_status);
}

void SimulationRuntime::restart() {
    stop();
    start();
}

void SimulationRuntime::step(int stepMs) {
    if (m_status == SimulationStatus::Stopped) {
        start();
        pause();
    }
    m_simulatedElapsedMs += stepMs;
    updateTimerDataSources(stepMs);
    updateCalculatedDataSources();
    updateVariableWaveforms(m_simulatedElapsedMs);
    emit clockTicked(m_simulatedElapsedMs);
}

void SimulationRuntime::setSpeedFactor(double factor) {
    factor = std::clamp(factor, 0.1, 20.0);
    if (std::abs(m_speedFactor - factor) > 0.01) {
        m_speedFactor = factor;
        if (m_status == SimulationStatus::Running) {
            int interval = static_cast<int>(50.0 / m_speedFactor);
            m_tickTimer->setInterval(std::max(5, interval));
        }
        emit speedFactorChanged(m_speedFactor);
    }
}

Screen* SimulationRuntime::activeScreen() const {
    if (!m_project) return nullptr;
    if (m_activeScreenId.isEmpty()) return m_project->activeScreen();
    return m_project->findScreen(m_activeScreenId);
}

void SimulationRuntime::setActiveScreenId(const QString& screenId) {
    if (m_activeScreenId != screenId && m_project && m_project->findScreen(screenId)) {
        m_activeScreenId = screenId;
        m_project->setActiveScreenById(screenId);
        emit activeScreenChanged(screenId);

        // Update all bindings on the newly active screen
        for (auto it = m_simulatedValues.begin(); it != m_simulatedValues.end(); ++it) {
            propagateDataSourceBindings(it.key(), it.value());
        }
    }
}

QVariant SimulationRuntime::dataSourceValue(const QString& sourceId) const {
    return m_simulatedValues.value(sourceId);
}

void SimulationRuntime::setDataSourceValue(const QString& sourceId, const QVariant& value) {
    QVariant oldVal = m_simulatedValues.value(sourceId);
    if (oldVal != value) {
        m_simulatedValues[sourceId] = value;
        propagateDataSourceBindings(sourceId, value);
        updateCalculatedDataSources();

        // Record log
        SimulationLogEntry log;
        log.timestampMs = m_simulatedElapsedMs;
        log.timeString = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        log.sourceId = sourceId;
        log.oldValue = oldVal;
        log.newValue = value;
        log.details = QString("DataSource '%1' updated to %2").arg(sourceId, value.toString());
        m_logEntries.append(log);
        if (m_logEntries.size() > 500) m_logEntries.removeFirst();

        emit dataSourceValueChanged(sourceId, value);
        emit logEntryAdded(log);
    }
}

void SimulationRuntime::propagateDataSourceBindings(const QString& sourceId, const QVariant& value) {
    if (!m_project || m_inBindingUpdate) return;
    m_inBindingUpdate = true;

    // Also update virtual hardware backend if this datasource maps to a hardware pin
    const DataSource* ds = m_project->findDataSource(sourceId);
    if (ds && !ds->hardwareRef().isEmpty() && m_backend) {
        QString pin = ds->hardwareRef();
        if (ds->type() == DataSourceType::Gpio) {
            m_backend->setSimulatedDigital(pin, value.toBool());
        } else if (ds->type() == DataSourceType::Adc || ds->type() == DataSourceType::Sensor) {
            double norm = value.toDouble();
            if (norm > 1.0) norm = norm / 100.0; // scale percentage
            m_backend->setSimulatedAdc(pin, static_cast<quint32>(norm * 4095.0), norm);
        } else if (ds->type() == DataSourceType::Pwm) {
            m_backend->setSimulatedPwm(pin, value.toDouble());
        }
    }

    // Find and update bound components across all project bindings
    for (const auto& bind : m_project->dataBindings()) {
        if (bind.sourceId() == sourceId && bind.direction() != BindingDirection::Write) {
            UIComponent* target = findComponent(bind.componentId());
            if (target) {
                applyComponentProperty(target, bind.propertyName(), value);
            }
        }
    }

    m_inBindingUpdate = false;
}

void SimulationRuntime::applyComponentProperty(UIComponent* comp, const QString& property, const QVariant& value) {
    if (!comp) return;

    QString propLower = property.toLower();

    // 1. Value visualization / progress components
    auto* valComp = dynamic_cast<ValueVisualizationComponent*>(comp);
    if (valComp && (propLower == "value" || propLower == "actualvalue")) {
        valComp->setValue(value.toDouble());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 2. Slider
    auto* slider = dynamic_cast<SliderComponent*>(comp);
    if (slider && propLower == "value") {
        slider->setValue(value.toInt());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 3. Switch & Checkbox
    auto* sw = dynamic_cast<SwitchComponent*>(comp);
    if (sw && (propLower == "checked" || propLower == "value")) {
        sw->setChecked(value.toBool());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }
    auto* cb = dynamic_cast<CheckboxComponent*>(comp);
    if (cb && (propLower == "checked" || propLower == "value")) {
        cb->setChecked(value.toBool());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 4. Label & TextInput
    auto* lbl = dynamic_cast<LabelComponent*>(comp);
    if (lbl && propLower == "text") {
        lbl->setText(value.toString());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }
    auto* txt = dynamic_cast<TextInputComponent*>(comp);
    if (txt && propLower == "text") {
        txt->setText(value.toString());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 5. Tab View & Navigation Bar
    auto* tab = dynamic_cast<TabViewComponent*>(comp);
    if (tab && (propLower == "activetabindex" || propLower == "selectedindex" || propLower == "value")) {
        tab->setActiveTabIndex(value.toInt());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }
    auto* nav = dynamic_cast<NavigationBarComponent*>(comp);
    if (nav && (propLower == "selectedindex" || propLower == "value")) {
        nav->setSelectedIndex(value.toInt());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 6. List & Table
    auto* list = dynamic_cast<ListComponent*>(comp);
    if (list && (propLower == "selectedindex" || propLower == "value")) {
        list->setSelectedIndex(value.toInt());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }
    auto* table = dynamic_cast<TableComponent*>(comp);
    if (table && (propLower == "selectedrow" || propLower == "selectedindex" || propLower == "value")) {
        table->setSelectedRow(value.toInt());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }

    // 7. Component State Style
    if (propLower == "state" || propLower == "currentstate") {
        comp->setCurrentState(value.toString());
        emit componentPropertyChanged(comp->componentId(), property, value);
        return;
    }
}

void SimulationRuntime::notifyComponentPropertyChanged(UIComponent* comp, const QString& propertyName, const QVariant& value) {
    if (!comp || !m_project || m_inBindingUpdate) return;

    // Check write bindings: Component -> DataSource
    for (const auto& bind : m_project->dataBindings()) {
        if (bind.componentId() == comp->componentId() &&
            bind.propertyName().toLower() == propertyName.toLower() &&
            bind.direction() != BindingDirection::Read)
        {
            setDataSourceValue(bind.sourceId(), value);
        }
    }
}

void SimulationRuntime::notifyComponentInteraction(UIComponent* comp, const QString& trigger) {
    if (!comp) return;

    // 1. Navigation Bar item target screen
    auto* nav = dynamic_cast<NavigationBarComponent*>(comp);
    if (nav && nav->selectedIndex() >= 0 && nav->selectedIndex() < nav->items().size()) {
        QString target = nav->items().at(nav->selectedIndex()).targetScreenId;
        if (!target.isEmpty() && m_project && m_project->findScreen(target)) {
            setActiveScreenId(target);
        }
    }

    // 2. Prototype Interactions configured on component
    for (const QJsonValue& val : comp->interactions()) {
        QJsonObject inter = val.toObject();
        if (inter.value("trigger").toString().compare(trigger, Qt::CaseInsensitive) != 0) {
            continue;
        }

        QString targetId = inter.value("target").toString();
        QString action = inter.value("action").toString();
        QString valueStr = inter.value("value").toString();

        // Check if target is a Screen
        if (m_project && m_project->findScreen(targetId)) {
            setActiveScreenId(targetId);
            continue;
        }

        // Target component interaction
        UIComponent* targetComp = findComponent(targetId);
        if (!targetComp) continue;

        if (action == "Show") {
            targetComp->setComponentVisible(true);
        } else if (action == "Hide") {
            targetComp->setComponentVisible(false);
        } else if (action == "Toggle") {
            auto* sw = dynamic_cast<SwitchComponent*>(targetComp);
            if (sw) {
                sw->setChecked(!sw->isChecked());
                notifyComponentPropertyChanged(sw, "checked", sw->isChecked());
            } else {
                targetComp->setComponentVisible(!targetComp->isComponentVisible());
            }
        } else if (action == "Set Value") {
            applyComponentProperty(targetComp, "value", valueStr);
        }
    }
}

UIComponent* SimulationRuntime::findComponent(const QString& compId) const {
    if (!m_project) return nullptr;

    // Check active screen first
    Screen* active = activeScreen();
    if (active) {
        for (UIComponent* c : active->components()) {
            if (c && c->componentId() == compId) return c;
        }
    }

    // Check all screens
    for (Screen* scr : m_project->screens()) {
        if (!scr || scr == active) continue;
        for (UIComponent* c : scr->components()) {
            if (c && c->componentId() == compId) return c;
        }
    }

    return nullptr;
}

void SimulationRuntime::onTickTimer() {
    int deltaMs = static_cast<int>(50 * m_speedFactor);
    m_simulatedElapsedMs += deltaMs;

    updateTimerDataSources(deltaMs);
    updateCalculatedDataSources();
    updateVariableWaveforms(m_simulatedElapsedMs);

    emit clockTicked(m_simulatedElapsedMs);
}

void SimulationRuntime::updateTimerDataSources(int deltaMs) {
    if (!m_project) return;

    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Timer) {
            int period = ds.metadata().value("periodMs").toInt(1000);
            if (period <= 0) period = 1000;
            int count = static_cast<int>(m_simulatedElapsedMs / period);
            setDataSourceValue(ds.id(), count);
        }
    }
}

void SimulationRuntime::updateCalculatedDataSources() {
    if (!m_project) return;

    // Prepare variable map
    QMap<QString, double> vars;
    for (auto it = m_simulatedValues.begin(); it != m_simulatedValues.end(); ++it) {
        vars[it.key()] = it.value().toDouble();
    }

    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Calculated) {
            QString expr = ds.metadata().value("expression").toString();
            if (!expr.isEmpty()) {
                double result = evaluateSafeExpression(expr, vars);
                setDataSourceValue(ds.id(), result);
            }
        }
    }
}

double SimulationRuntime::evaluateSafeExpression(const QString& expr, const QMap<QString, double>& variables) {
    QString clean = expr.trimmed();
    if (clean.isEmpty()) return 0.0;

    // Direct variable or number
    if (variables.contains(clean)) {
        return variables.value(clean);
    }
    bool isNum = false;
    double directNum = clean.toDouble(&isNum);
    if (isNum) return directNum;

    // Tokenize into simple tokens: numbers, identifiers, operators +, -, *, /, (, )
    struct Token {
        enum Type { Number, Ident, Plus, Minus, Mul, Div, LParen, RParen, End } type;
        double numVal = 0.0;
        QString text;
    };

    QList<Token> tokens;
    int i = 0;
    while (i < clean.size()) {
        QChar c = clean[i];
        if (c.isSpace()) {
            i++;
            continue;
        }
        if (c == '+') { tokens.append({Token::Plus, 0.0, "+"}); i++; }
        else if (c == '-') { tokens.append({Token::Minus, 0.0, "-"}); i++; }
        else if (c == '*') { tokens.append({Token::Mul, 0.0, "*"}); i++; }
        else if (c == '/') { tokens.append({Token::Div, 0.0, "/"}); i++; }
        else if (c == '(') { tokens.append({Token::LParen, 0.0, "("}); i++; }
        else if (c == ')') { tokens.append({Token::RParen, 0.0, ")"}); i++; }
        else if (c.isDigit() || c == '.') {
            int start = i;
            while (i < clean.size() && (clean[i].isDigit() || clean[i] == '.')) i++;
            double val = clean.mid(start, i - start).toDouble();
            tokens.append({Token::Number, val, ""});
        } else if (c.isLetter() || c == '_') {
            int start = i;
            while (i < clean.size() && (clean[i].isLetterOrNumber() || clean[i] == '_')) i++;
            QString id = clean.mid(start, i - start);
            tokens.append({Token::Ident, 0.0, id});
        } else {
            i++;
        }
    }
    tokens.append({Token::End, 0.0, ""});

    int pos = 0;
    auto peek = [&]() -> Token { return (pos < tokens.size()) ? tokens[pos] : Token{Token::End, 0.0, ""}; };
    auto consume = [&]() -> Token { return (pos < tokens.size()) ? tokens[pos++] : Token{Token::End, 0.0, ""}; };

    std::function<double()> parseExpression;
    std::function<double()> parseTerm;
    std::function<double()> parseFactor;

    parseFactor = [&]() -> double {
        Token tok = consume();
        if (tok.type == Token::Number) {
            return tok.numVal;
        } else if (tok.type == Token::Ident) {
            return variables.value(tok.text, 0.0);
        } else if (tok.type == Token::LParen) {
            double val = parseExpression();
            if (peek().type == Token::RParen) consume();
            return val;
        } else if (tok.type == Token::Minus) {
            return -parseFactor();
        } else if (tok.type == Token::Plus) {
            return parseFactor();
        }
        return 0.0;
    };

    parseTerm = [&]() -> double {
        double left = parseFactor();
        while (peek().type == Token::Mul || peek().type == Token::Div) {
            Token op = consume();
            double right = parseFactor();
            if (op.type == Token::Mul) {
                left *= right;
            } else {
                left = (right != 0.0) ? (left / right) : 0.0;
            }
        }
        return left;
    };

    parseExpression = [&]() -> double {
        double left = parseTerm();
        while (peek().type == Token::Plus || peek().type == Token::Minus) {
            Token op = consume();
            double right = parseTerm();
            if (op.type == Token::Plus) {
                left += right;
            } else {
                left -= right;
            }
        }
        return left;
    };

    return parseExpression();
}

QStringList SimulationRuntime::availablePresets() const {
    return {
        "Normal Operation",
        "Low Battery Warning",
        "High Temperature Alert",
        "Engine Redline",
        "Pressure Warning"
    };
}

void SimulationRuntime::applyPreset(const QString& presetName) {
    auto setMulti = [&](const QStringList& names, const QVariant& val) {
        for (const QString& n : names) {
            if (m_simulatedValues.contains(n)) {
                setDataSourceValue(n, val);
            }
        }
    };

    if (presetName == "Normal Operation") {
        setMulti({"src_speed", "speed_source", "speed"}, 80.0);
        setMulti({"src_rpm", "rpm_source", "rpm"}, 2800.0);
        setMulti({"src_temp", "temperature_source", "temp", "temperature"}, 85.0);
        setMulti({"src_battery", "battery_source", "battery"}, 90.0);
        setMulti({"src_pressure", "pressure_source", "pressure"}, 3.2);
        setMulti({"src_pot", "pot_source", "adc_source"}, 0.5);
    } else if (presetName == "Low Battery Warning" || presetName == "Low Battery") {
        setMulti({"src_speed", "speed_source", "speed"}, 35.0);
        setMulti({"src_rpm", "rpm_source", "rpm"}, 1200.0);
        setMulti({"src_battery", "battery_source", "battery"}, 12.0);
    } else if (presetName == "High Temperature Alert" || presetName == "High Temperature") {
        setMulti({"src_speed", "speed_source", "speed"}, 105.0);
        setMulti({"src_rpm", "rpm_source", "rpm"}, 3800.0);
        setMulti({"src_temp", "temperature_source", "temp", "temperature"}, 118.0);
    } else if (presetName == "Engine Redline") {
        setMulti({"src_speed", "speed_source", "speed"}, 215.0);
        setMulti({"src_rpm", "rpm_source", "rpm"}, 7500.0);
    } else if (presetName == "Pressure Warning") {
        setMulti({"src_pressure", "pressure_source", "pressure"}, 0.7);
    }
}

void SimulationRuntime::clearLogs() {
    m_logEntries.clear();
}

void SimulationRuntime::updateVariableWaveforms(qint64 elapsedMs) {
    if (!m_project) return;
    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Variable && ds.variableConfig().waveform != SimulationWaveform::None) {
            double val = ds.variableConfig().evaluateWaveform(elapsedMs);
            setDataSourceValue(ds.id(), val);
        }
    }
}

void SimulationRuntime::injectCanFrame(quint32 messageId, const QByteArray& payload) {
    if (!m_project) return;
    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Can && ds.canConfig().messageId == messageId) {
            double val = ds.canConfig().decodePayload(payload);
            setDataSourceValue(ds.id(), val);
        }
    }
}

void SimulationRuntime::injectUartStream(const QString& rawFrame) {
    if (!m_project) return;
    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Uart) {
            QVariant parsed = ds.uartConfig().parseIncomingText(rawFrame);
            if (parsed.isValid()) {
                setDataSourceValue(ds.id(), parsed);
            }
        }
    }
}

void SimulationRuntime::injectModbusRegisters(int slaveId, ModbusRegisterType regType, int startAddress, const QVector<quint16>& rawRegs) {
    if (!m_project) return;
    for (const auto& ds : m_project->dataSources()) {
        if (ds.type() == DataSourceType::Modbus &&
            ds.modbusConfig().slaveId == slaveId &&
            ds.modbusConfig().registerType == regType &&
            ds.modbusConfig().address >= startAddress &&
            ds.modbusConfig().address < startAddress + rawRegs.size())
        {
            int offset = ds.modbusConfig().address - startAddress;
            QVector<quint16> slice = rawRegs.mid(offset);
            QVariant parsed = ds.modbusConfig().decodeRegisters(slice);
            if (parsed.isValid()) {
                setDataSourceValue(ds.id(), parsed);
            }
        }
    }
}

} // namespace Simulator
