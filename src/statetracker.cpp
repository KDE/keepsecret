// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "statetracker.h"
#include "kwallets_debug.h"

#include <KLocalizedString>
#include <cstdlib>
#include <memory>

class StateTrackerSingleton
{
public:
    StateTrackerSingleton();
    std::unique_ptr<StateTracker> stateTracker;
};

StateTrackerSingleton::StateTrackerSingleton()
    : stateTracker(std::make_unique<StateTracker>())
{
}

Q_GLOBAL_STATIC(StateTrackerSingleton, s_stateTracker)

StateTracker::StateTracker(QObject *parent)
    : QObject(parent)
{
}

StateTracker::~StateTracker()
{
}

StateTracker *StateTracker::instance()
{
    return s_stateTracker->stateTracker.get();
}

StateTracker::Status StateTracker::status() const
{
    return m_status;
}

void StateTracker::setStatus(Status status)
{
    if (status == m_status) {
        return;
    }

    qCDebug(KWALLETS_LOG) << "Setting status" << status;

    const Status oldStatus = m_status;
    m_status = status;
    Q_EMIT statusChanged(oldStatus, status);
    if ((oldStatus & ServiceConnected) != (status & ServiceConnected)) {
        Q_EMIT serviceConnectedChanged(status & ServiceConnected);
    }
}

void StateTracker::setState(StateTracker::State state)
{
    setStatus(m_status | state);
}

void StateTracker::clearState(StateTracker::State state)
{
    setStatus(m_status & ~state);
}

bool StateTracker::isServiceConnected() const
{
    return m_status & ServiceConnected;
}

StateTracker::Operations StateTracker::operations() const
{
    return m_operations;
}

void StateTracker::setOperations(StateTracker::Operations operations)
{
    if (operations == m_operations) {
        return;
    }

    qCDebug(KWALLETS_LOG) << "Setting operations" << operations;

    const Operations oldOperations = m_operations;
    m_operations = operations;
    Q_EMIT operationsChanged(oldOperations, operations);

    // If we are not saving anymore, remove ItemNeedsSave from the status
    if (!(operations & ItemSaving)) {
        setStatus(m_status & ~ItemNeedsSave);
    }
}

void StateTracker::setOperation(StateTracker::Operation operation)
{
    setOperations(m_operations | operation);
}

void StateTracker::clearOperation(StateTracker::Operation operation)
{
    // Keep the Loading or Saving flags if we still have another load/save operation
    Operations result = m_operations & ~operation;
    if (result & (ItemSavingLabel | ItemSavingSecret | ItemSavingAttributes)) {
        result |= ItemSaving;
    }
    if (result & (ItemLoadingSecret | ItemUnlocking)) {
        result |= ItemLoading;
    }
    setOperations(result);
}

StateTracker::Error StateTracker::error() const
{
    return m_error;
}

QString StateTracker::errorMessage() const
{
    return m_errorMessage;
}

void StateTracker::setError(StateTracker::Error error, const QString &errorMessage)
{
    if (error != NoError) {
        qCWarning(KWALLETS_LOG) << "ERROR:" << error << errorMessage;
    }
    if (error != m_error || errorMessage != m_errorMessage) {
        m_error = error;
        m_errorMessage = errorMessage;
        Q_EMIT errorChanged(error, errorMessage);
    }
}

void StateTracker::clearError()
{
    setError(NoError, QString());
}

#include "moc_statetracker.cpp"
