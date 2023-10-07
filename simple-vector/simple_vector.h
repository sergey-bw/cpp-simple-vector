#pragma once

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity_to_reserve) noexcept
        : capacity_to_reserve_(capacity_to_reserve)
    {
    }

    size_t GetCapacityToReserve() const noexcept {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Конструкрор с резервированием памяти под элементы
    explicit SimpleVector(const ReserveProxyObj& obj)
        : items_(ArrayPtr<Type>(obj.GetCapacityToReserve()))
        , capacity_(obj.GetCapacityToReserve())
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type(0))
    {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : items_ (ArrayPtr<Type>(size))
    {
        for (auto it = begin(); it != end() + size; ++it) {
            *it = std::move(Type(value));
        }
        size_ = capacity_ = size;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : items_ (ArrayPtr<Type>(init.size()))
    {
        std::copy(std::make_move_iterator(init.begin())
            , std::make_move_iterator(init.end()), begin());
        size_ = capacity_ = init.size();
    }

    // Конструктор копирования
    SimpleVector(const SimpleVector& other) {
        SimpleVector<Type> tmp(other.GetSize());
        std::copy(std::make_move_iterator(other.begin())
            , std::make_move_iterator(other.end()), tmp.begin());
        swap(tmp);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this != rhs) {
            SimpleVector<Type> tmp(rhs);
            swap(tmp);
        }
        return *this;
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other){
        // Используем оператор перемещиния
        *this = std::move(other);
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            items_ = std::move(rhs.items_);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this;
    }

    void Reserve(size_t new_capacity){
        if (new_capacity <= capacity_) {
            return;
        }

        ArrayPtr<Type> tmp(new_capacity);
        std::copy(std::make_move_iterator(begin())
            , std::make_move_iterator(end()), &tmp[0]);
        items_.swap(tmp);
        capacity_ = new_capacity;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    template <typename Args>
    void PushBack(Args item) {
        if (size_ < capacity_) {
            items_[size_] = std::move(item);
            ++size_;
            return;
        }

        size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
        ArrayPtr<Type> tmp(new_capacity);
        std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), &tmp[0]);
        tmp[size_] = std::move(item);
        items_.swap(tmp);
        ++size_;
        capacity_ = new_capacity;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    template <typename Args>
    Iterator Insert(ConstIterator pos, Args value) {
        if (pos < cbegin() || pos > cend()) {
            throw std::out_of_range("iterator out of range");
        }
        size_t index = std::distance(cbegin(), pos);
    
        if (size_ < capacity_) {
            std::copy_backward(std::make_move_iterator(begin() + index)
                , std::make_move_iterator(end()), end() + 1);
            items_[index] = std::move(value);
        } else {
            size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(std::make_move_iterator(begin())
                , std::make_move_iterator(begin() + index), &tmp[0]);
            std::copy_backward(std::make_move_iterator(begin() + index)
                , std::make_move_iterator(end()), &tmp[size_ + 1]);
            tmp[index] = std::move(value);
            items_.swap(tmp);
            capacity_ = new_capacity;
        }

        ++size_;
        return begin() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ == 0) {
            return;
        }
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    //template <typename Args>
    Iterator Erase(ConstIterator pos) {
        if (pos < cbegin() || pos > cend()) {
            throw std::out_of_range("iterator out of range");
        }
        size_t index = std::distance(cbegin(), pos);
        std::copy(std::make_move_iterator(begin() + index + 1)
            , std::make_move_iterator(end()), begin() + index);
        --size_;
        return begin() + index;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(this->size_, other.size_);
        std::swap(this->capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }

        if (new_size <= capacity_) {
            if (std::is_copy_constructible<Type>::value) {
                for (auto it = end(); it != end() + (new_size - size_); ++it) {
                    *it = std::move(Type(0));
                }
            }
            size_ = new_size;
            return;
        }

        size_t new_capacity = std::max(new_size, capacity_ * 2);
        ArrayPtr<Type> tmp(new_capacity);
        std::copy(std::make_move_iterator(begin())
            , std::make_move_iterator(end()), &tmp[0]);
        //std::fill(&tmp[size_], &tmp[new_size], static_cast<Type>(0));
        if (std::is_copy_constructible<Type>::value) {
            for (auto it = &tmp[size_]; it != &tmp[new_size]; ++it) {
                *it = std::move(Type(0));
            }
        }
        items_.swap(tmp);
        size_ = new_size;
        capacity_ = new_capacity; 
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return &items_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return &items_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }
private:
    ArrayPtr<Type> items_;
    size_t capacity_ = 0;
    size_t size_ = 0;
};

template <typename Type>
void swap(SimpleVector<Type>& lhs, SimpleVector<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize()
        && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 