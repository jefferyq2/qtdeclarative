/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debugging.h"
#include "qmljs_objects.h"

#include <iostream>

using namespace QQmlJS;
using namespace QQmlJS::Debugging;

FunctionState::FunctionState(VM::ExecutionContext *context)
    : _context(context)
{
    if (debugger())
        debugger()->enterFunction(this);
}

FunctionState::~FunctionState()
{
    if (debugger())
        debugger()->leaveFunction(this);
}

VM::Value *FunctionState::argument(unsigned idx)
{
    if (idx < _context->lexicalEnvironment->argumentCount)
        return _context->lexicalEnvironment->arguments + idx;
    else
        return 0;
}

VM::Value *FunctionState::local(unsigned idx)
{
    if (idx < _context->lexicalEnvironment->variableCount())
        return _context->lexicalEnvironment->locals + idx;
    return 0;
}

Debugger::Debugger(VM::ExecutionEngine *engine)
    : _engine(engine)
{
}

Debugger::~Debugger()
{
    qDeleteAll(_functionInfo.values());
}

void Debugger::addFunction(IR::Function *function)
{
    _functionInfo.insert(function, new FunctionDebugInfo(function));
}

void Debugger::addaddBasicBlockOffset(IR::Function *function, IR::BasicBlock *block, ptrdiff_t blockOffset)
{
    _functionInfo[function]->addBasicBlockOffset(block, blockOffset);
}

void Debugger::setSourceLocation(IR::Function *function, unsigned line, unsigned column)
{
    _functionInfo[function]->setSourceLocation(line, column);
}

FunctionDebugInfo *Debugger::debugInfo(VM::FunctionObject *function) const
{
    if (!function)
        return 0;

    if (VM::ScriptFunction *sf = function->asScriptFunction())
        return _functionInfo[sf->function];
    else
        return 0;
}

QString Debugger::name(VM::FunctionObject *function) const
{
    if (FunctionDebugInfo *i = debugInfo(function))
        if (i->function)
            if (const QString *n = i->function->name)
                return *n;

    return QString();
}

void Debugger::aboutToCall(VM::FunctionObject *function, VM::ExecutionContext *context)
{
    _callStack.append(CallInfo(context, function));
}

void Debugger::justLeft(VM::ExecutionContext *context)
{
    int idx = callIndex(context);
    if (idx < 0)
        qDebug() << "Oops, leaving a function that was not registered...?";
    else
        _callStack.resize(idx);
}

void Debugger::enterFunction(FunctionState *state)
{
    _callStack[callIndex(state->context())].state = state;

#ifdef DO_TRACE_INSTR
    QString n = name(_callStack[callIndex(state->context())].function);
    std::cerr << "*** Entering \"" << qPrintable(n) << "\" with" << state->context()->variableEnvironment->argumentCount << "args" << std::endl;
    for (unsigned i = 0; i < state->context()->variableEnvironment->argumentCount; ++i)
        std::cerr << "        " << i << ": " << currentArg(i) << std::endl;
#endif // DO_TRACE_INSTR
}

void Debugger::leaveFunction(FunctionState *state)
{
    _callStack[callIndex(state->context())].state = 0;
}

void Debugger::aboutToThrow(VM::Value *value)
{
    qDebug() << "*** We are about to throw...:" << value->toString(currentState()->context())->toQString();
}

FunctionState *Debugger::currentState() const
{
    if (_callStack.isEmpty())
        return 0;
    else
        return _callStack.last().state;
}

const char *Debugger::currentArg(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->argument(idx)->toString(state->context())->toQString());
}

const char *Debugger::currentLocal(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->local(idx)->toString(state->context())->toQString());
}

const char *Debugger::currentTemp(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->temp(idx)->toString(state->context())->toQString());
}

int Debugger::callIndex(VM::ExecutionContext *context)
{
    for (int idx = _callStack.size() - 1; idx >= 0; --idx) {
        if (_callStack[idx].context == context)
            return idx;
    }

    return -1;
}
