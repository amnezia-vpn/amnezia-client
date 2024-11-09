#pragma once
#include <Windows.h>
#include <utility>

template<typename T, auto Close>
class RAIIHandle {
public:
    RAIIHandle() : m_handle(NULL) {}
    RAIIHandle(T handle) : m_handle(handle) {}

    RAIIHandle(const RAIIHandle &) = delete;
    RAIIHandle &operator=(const RAIIHandle &) = delete;

    RAIIHandle(RAIIHandle &&other) : m_handle(std::exchange(other.m_handle, NULL)) {}
    RAIIHandle &operator=(RAIIHandle &&other) {
        std::swap(m_handle, other.m_handle);
        return *this;
    }

    ~RAIIHandle() {
        if (m_handle) {
            Close(m_handle);
            m_handle = NULL;
        }
    }

    operator T() const { return m_handle; }

    operator bool() const { return m_handle != NULL && m_handle != INVALID_HANDLE_VALUE; }

private:
    T m_handle;
};

using ServiceHandle = RAIIHandle<SC_HANDLE, CloseServiceHandle>;
using WinHandle = RAIIHandle<HANDLE, CloseHandle>;
