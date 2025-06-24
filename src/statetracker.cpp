// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

#include "statetracker.h"
#include "kwallets_debug.h"

#include <KLocalizedString>
#include <cstdlib>

StateTracker::StateTracker(QObject *parent)
    : QObject(parent)
{
}

StateTracker::~StateTracker()
{
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

    qWarning() << "Setting status" << status;

    m_status = status;
    Q_EMIT statusChanged(status);
}

void StateTracker::setState(StateTracker::State state)
{
    setStatus(m_status | state);
}

void StateTracker::clearState(StateTracker::State state)
{
    setStatus(m_status & ~state);
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

    qWarning() << "Setting operations" << operations;

    m_operations = operations;
    Q_EMIT operationsChanged(operations);
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
    if (error != m_error) {
        m_error = error;
        Q_EMIT errorChanged(error);
    }

    if (errorMessage != m_errorMessage) {
        m_errorMessage = errorMessage;
        Q_EMIT errorMessageChanged(errorMessage);
    }
}

void StateTracker::clearError()
{
    if (m_error != NoError) {
        m_error = NoError;
        Q_EMIT errorChanged(NoError);
    }

    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        Q_EMIT errorMessageChanged(QString());
    }
}

#include "moc_statetracker.cpp"
