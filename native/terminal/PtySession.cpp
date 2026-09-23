// Modified: 2026-09-23-terminal-candidate; see docs/ai/changes/2026-09-23-terminal-candidate.json
// SPDX-License-Identifier: MIT
// AI-Change: 2026-09-22-native-foundation (OpenAI / GPT-6 Astra Pro)
// Provenance: docs/ai/changes/2026-09-22-native-foundation.json
#include "PtySession.h"
#include <QDir>
#include <QFileInfo>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace mterm {
#ifdef Q_OS_WIN
namespace {
void closeHandle(HANDLE &handle) {
    if (handle && handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = nullptr;
    }
}
std::wstring quote(const QString &value) {
    const auto arg = value.toStdWString();
    if (!value.isEmpty() && !value.contains(' ') && !value.contains('\t') && !value.contains('"'))
        return arg;
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (const auto ch : arg) {
        if (ch == L'\\') {
            ++slashes;
            continue;
        }
        if (ch == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
            result += ch;
        } else {
            result.append(slashes, L'\\');
            result += ch;
        }
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result += L'\"';
    return result;
}
} // namespace
struct PtySession::Impl {
    HPCON console = nullptr;
    HANDLE input = nullptr, output = nullptr, process = nullptr, job = nullptr, reader = nullptr,
           writer = nullptr;
    std::atomic<bool> closing{false};
    std::mutex mutex;
    std::condition_variable condition;
    QByteArray pendingOutput, pendingInput;
    bool active = false;
    static DWORD WINAPI readLoop(void *value) {
        auto *self = static_cast<Impl *>(value);
        char buffer[8192];
        DWORD count = 0;
        while (ReadFile(self->output, buffer, sizeof(buffer), &count, nullptr) && count) {
            std::unique_lock lock(self->mutex);
            self->condition.wait(lock, [&] {
                return self->closing.load() || self->pendingOutput.size() + count <= 1048576;
            });
            if (!self->closing.load())
                self->pendingOutput.append(buffer, qsizetype(count));
        }
        return 0;
    }
    static DWORD WINAPI writeLoop(void *value) {
        auto *self = static_cast<Impl *>(value);
        while (!self->closing.load()) {
            QByteArray bytes;
            {
                std::unique_lock lock(self->mutex);
                self->condition.wait(
                    lock, [&] { return self->closing.load() || !self->pendingInput.isEmpty(); });
                if (self->closing.load())
                    break;
                bytes = std::move(self->pendingInput);
                self->pendingInput.clear();
            }
            qsizetype offset = 0;
            while (offset < bytes.size() && !self->closing.load()) {
                DWORD written = 0;
                if (!WriteFile(self->input, bytes.constData() + offset,
                               DWORD(bytes.size() - offset), &written, nullptr) ||
                    !written)
                    return 0;
                offset += written;
            }
        }
        return 0;
    }
};
#else
struct PtySession::Impl {
    bool active = false;
};
#endif
PtySession::PtySession(QObject *parent) : QObject(parent) {
    // Timers must migrate with the PTY when its service is moved to the worker.
    drain_.setParent(this);
    poll_.setParent(this);
    drain_.setInterval(16);
    poll_.setInterval(100);
    connect(&drain_, &QTimer::timeout, this, [this] {
#ifdef Q_OS_WIN
        if (!impl_)
            return;
        QByteArray bytes;
        {
            std::lock_guard lock(impl_->mutex);
            bytes = impl_->pendingOutput.left(65536);
            impl_->pendingOutput.remove(0, bytes.size());
        }
        impl_->condition.notify_all();
        if (!bytes.isEmpty())
            emit dataReady(bytes);
#endif
    });
    connect(&poll_, &QTimer::timeout, this, [this] {
#ifdef Q_OS_WIN
        if (impl_ && impl_->active && WaitForSingleObject(impl_->process, 0) == WAIT_OBJECT_0) {
            DWORD code = 0;
            GetExitCodeProcess(impl_->process, &code);
            finish(int(code));
        }
#endif
    });
}
PtySession::~PtySession() {
    disconnect(this, nullptr, nullptr, nullptr);
    finish(-1);
}
bool PtySession::start(const QString &executable, const QStringList &arguments, const QString &cwd,
                       int columns, int rows) {
    if (running() || columns < 1 || columns > 400 || rows < 1 || rows > 200 ||
        !QDir(cwd).exists() || !QFileInfo(executable).isFile()) {
        error_ = "Invalid PTY executable, directory, dimensions or active state";
        return false;
    }
    if (arguments.size() > 64 || executable.contains(QChar::Null))
        return false;
    for (const auto &argument : arguments)
        if (argument.contains(QChar::Null) || argument.size() > 4096)
            return false;
#ifdef Q_OS_WIN
    error_.clear();
    impl_ = std::make_unique<Impl>();
    auto fail = [this](const QString &operation) {
        error_ = operation + QString(" (Windows error %1)").arg(GetLastError());
        finish(-1);
        return false;
    };
    HANDLE inputRead = nullptr, outputWrite = nullptr;
    if (!CreatePipe(&inputRead, &impl_->input, nullptr, 0))
        return fail("Create input pipe");
    if (!CreatePipe(&impl_->output, &outputWrite, nullptr, 0)) {
        closeHandle(inputRead);
        return fail("Create output pipe");
    }
    const auto hr = CreatePseudoConsole({SHORT(columns), SHORT(rows)}, inputRead, outputWrite, 0,
                                        &impl_->console);
    closeHandle(inputRead);
    closeHandle(outputWrite);
    if (FAILED(hr))
        return fail("CreatePseudoConsole");
    impl_->reader = CreateThread(nullptr, 0, &Impl::readLoop, impl_.get(), 0, nullptr);
    if (!impl_->reader)
        return fail("Create PTY reader");
    impl_->writer = CreateThread(nullptr, 0, &Impl::writeLoop, impl_.get(), 0, nullptr);
    if (!impl_->writer)
        return fail("Create PTY writer");
    impl_->job = CreateJobObjectW(nullptr, nullptr);
    if (!impl_->job)
        return fail("Create PTY job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(impl_->job, JobObjectExtendedLimitInformation, &limits,
                                 sizeof(limits)))
        return fail("Configure PTY job");
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    std::vector<unsigned char> storage(attributeBytes);
    auto *attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes))
        return fail("Initialize process attributes");
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   impl_->console, sizeof(HPCON), nullptr, nullptr)) {
        DeleteProcThreadAttributeList(attributes);
        return fail("Assign pseudoconsole attribute");
    }
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    // Null standard handles prevent redirected parent stdio from bypassing ConPTY.
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process{};
    const auto application = QDir::toNativeSeparators(executable).toStdWString();
    auto command = quote(QDir::toNativeSeparators(executable));
    for (const auto &argument : arguments) {
        command += L' ';
        command += quote(argument);
    }
    const auto directory = QDir::toNativeSeparators(cwd).toStdWString();
    const bool created =
        CreateProcessW(application.c_str(), command.data(), nullptr, nullptr, FALSE,
                       EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                       nullptr, directory.c_str(), &startup.StartupInfo, &process);
    DeleteProcThreadAttributeList(attributes);
    if (!created)
        return fail("Create PTY shell");
    impl_->process = process.hProcess;
    // Establish process ownership before the shell can execute any user code.
    if (!AssignProcessToJobObject(impl_->job, process.hProcess)) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        return fail("Assign PTY process ownership");
    }
    if (ResumeThread(process.hThread) == DWORD(-1)) {
        CloseHandle(process.hThread);
        return fail("Resume PTY shell");
    }
    CloseHandle(process.hThread);
    impl_->active = true;
    drain_.start();
    poll_.start();
    return true;
#else
    Q_UNUSED(arguments);
    error_ = "Native PTY transport is currently Windows-only";
    return false;
#endif
}
bool PtySession::write(const QByteArray &bytes) {
#ifdef Q_OS_WIN
    if (!running() || bytes.size() > 65536)
        return false;
    {
        std::lock_guard lock(impl_->mutex);
        if (impl_->pendingInput.size() + bytes.size() > 65536)
            return false;
        impl_->pendingInput.append(bytes);
    }
    impl_->condition.notify_all();
    return true;
#else
    Q_UNUSED(bytes);
    return false;
#endif
}
bool PtySession::resize(int columns, int rows) {
#ifdef Q_OS_WIN
    return running() && columns >= 1 && columns <= 400 && rows >= 1 && rows <= 200 &&
           SUCCEEDED(ResizePseudoConsole(impl_->console, {SHORT(columns), SHORT(rows)}));
#else
    Q_UNUSED(columns);
    Q_UNUSED(rows);
    return false;
#endif
}
bool PtySession::running() const {
    return impl_ && impl_->active;
}
void PtySession::setOutputPaused(bool paused) {
    if (paused)
        drain_.stop();
    else if (running() && !drain_.isActive())
        drain_.start();
}
void PtySession::stop() {
    finish(-1);
}
void PtySession::finish(int code) {
    drain_.stop();
    poll_.stop();
    if (!impl_)
        return;
    const bool wasActive = impl_->active;
    impl_->active = false;
#ifdef Q_OS_WIN
    impl_->closing = true;
    impl_->condition.notify_all();
    closeHandle(impl_->job);
    // Keep the output reader alive while ClosePseudoConsole drains its final frame.
    if (impl_->console) {
        ClosePseudoConsole(impl_->console);
        impl_->console = nullptr;
    }
    if (impl_->reader) {
        CancelSynchronousIo(impl_->reader);
        WaitForSingleObject(impl_->reader, INFINITE);
        closeHandle(impl_->reader);
    }
    if (impl_->writer) {
        CancelSynchronousIo(impl_->writer);
        WaitForSingleObject(impl_->writer, INFINITE);
        closeHandle(impl_->writer);
    }
    const auto last = impl_->pendingOutput;
    if (!last.isEmpty())
        emit dataReady(last);
    closeHandle(impl_->input);
    closeHandle(impl_->output);
    closeHandle(impl_->process);
#endif
    impl_.reset();
    if (wasActive)
        emit exited(code);
}
} // namespace mterm
