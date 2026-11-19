#pragma once
#ifndef CATA_SRC_SAFE_REFERENCE_H
#define CATA_SRC_SAFE_REFERENCE_H

















#include <memory>

template<typename T>
class safe_reference
{
    public:
        safe_reference() = default;

        T *get() const {
            return impl.lock().get();
        }

        explicit operator bool() const {
            return !!*this;
        }

        bool operator!() const {
            return impl.expired();
        }

        T &operator*() const {
            return *get();
        }

        T *operator->() const {
            return get();
        }
    private:
        friend class safe_reference_anchor;

        explicit safe_reference( const std::shared_ptr<T> &p ) : impl( p ) {}

        std::weak_ptr<T> impl;
};

class safe_reference_anchor
{
    public:
        safe_reference_anchor();
        safe_reference_anchor( const safe_reference_anchor & );
        safe_reference_anchor( safe_reference_anchor && ) noexcept;
        safe_reference_anchor &operator=( const safe_reference_anchor & );
        safe_reference_anchor &operator=( safe_reference_anchor && ) noexcept;

        template<typename T>
        safe_reference<T> reference_to( T *object ) {

            return safe_reference<T>( std::shared_ptr<T>( impl, object ) );
        }
    private:
        struct empty {};
        std::shared_ptr<empty> impl;
};

#endif
