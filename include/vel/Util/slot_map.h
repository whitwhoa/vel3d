#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>


struct slot_handle
{
    static constexpr std::uint32_t invalid_index = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t index = invalid_index;
    std::uint32_t generation = 0;

    constexpr explicit operator bool() const noexcept
    {
        return this->index != invalid_index;
    }

    friend constexpr bool operator==(const slot_handle a, const slot_handle b) noexcept
    {
        return a.index == b.index && a.generation == b.generation;
    }

    friend constexpr bool operator!=(const slot_handle a, const slot_handle b) noexcept
    {
        return !(a == b);
    }
};


static_assert(sizeof(slot_handle) == 8);


template <typename T>
class slot_map
{
    static_assert(
        std::is_nothrow_move_constructible_v<T>,
        "slot_map<T> requires T to be nothrow move-constructible."
        );

    static_assert(
        std::is_nothrow_move_assignable_v<T>,
        "slot_map<T> requires T to be nothrow move-assignable."
        );


public:

    using value_type = T;
    using size_type = std::size_t;

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;


private:

    struct slot_record
    {
        // If this is invalid_index, the slot is unused.
        std::uint32_t dense_index = slot_handle::invalid_index;

        // Incremented whenever this slot is freed.
        std::uint32_t generation = 1;

        // Used only while this slot is free.
        std::uint32_t next_free = slot_handle::invalid_index;
    };


    // Packed object storage.
    std::vector<T> dense;

    // dense index -> slot index
    std::vector<std::uint32_t> dense_to_slot;

    // slot index -> dense index
    std::vector<slot_record> slots;

    // Linked free-list contained inside slots.
    std::uint32_t free_head = slot_handle::invalid_index;


public:

    slot_map() = default;


    explicit slot_map(const size_type capacity)
    {
        this->reserve(capacity);
    }


    // Copying would duplicate the handle namespace, which is generally
    // undesirable for a slot map.
    slot_map(const slot_map&) = delete;
    slot_map& operator=(const slot_map&) = delete;

    slot_map(slot_map&&) noexcept = default;
    slot_map& operator=(slot_map&&) noexcept = default;


    void reserve(const size_type capacity)
    {
        this->dense.reserve(capacity);
        this->dense_to_slot.reserve(capacity);
        this->slots.reserve(capacity);
    }


    template <typename... Args>
    slot_handle emplace(Args&&... args)
    {
        std::uint32_t slot_index;


        if (this->free_head != slot_handle::invalid_index)
        {
            // Reuse an existing free slot.
            slot_index = this->free_head;
        }
        else
        {
            // Create a brand new slot.
            slot_index = static_cast<std::uint32_t>(this->slots.size());
        }


        const std::uint32_t dense_index = static_cast<std::uint32_t>(this->dense.size());


        // Construct the actual object in packed storage.
        this->dense.emplace_back(std::forward<Args>(args)...);


        // Record which slot owns this dense element.
        this->dense_to_slot.push_back(slot_index);


        if (this->free_head != slot_handle::invalid_index)
        {
            slot_record& slot = this->slots[slot_index];

            // Remove slot from free list.
            this->free_head = slot.next_free;

            slot.dense_index = dense_index;

            slot.next_free = slot_handle::invalid_index;

            return {
                slot_index,
                slot.generation
            };
        }


        // No free slot existed, so add a new one.
        slot_record slot;

        slot.dense_index = dense_index;


        this->slots.push_back(slot);


        return {
            slot_index,
            slot.generation
        };
    }


    slot_handle insert(const T& value)
    {
        return this->emplace(value);
    }


    slot_handle insert(T&& value)
    {
        return this->emplace(std::move(value));
    }


    // -------------------------------------------------------------------------
    // Erase by stable handle.
    // -------------------------------------------------------------------------

    bool erase(const slot_handle handle)
    {
        if (!this->contains(handle))
            return false;


        const std::uint32_t dense_index = this->slots[handle.index].dense_index;


        this->erase_dense_index(dense_index);


        return true;
    }


    // -------------------------------------------------------------------------
    // Erase by iterator.
    //
    // This is useful while iterating the packed storage.
    //
    // Because deletion uses swap-and-pop compaction, the final object may be
    // moved into the erased object's position.
    //
    // The returned iterator points to that moved object.
    //
    // If the erased object was already the final object, end() is returned.
    // -------------------------------------------------------------------------

    iterator erase(const_iterator it)
    {
        const size_type dense_index = static_cast<size_type>(it - this->dense.cbegin());


        if (dense_index >= this->dense.size())
            return this->dense.end();


        this->erase_dense_index(static_cast<std::uint32_t>(dense_index));


        // If another object was moved into the erased position,
        // this points to that object.
        //
        // If the erased object was the final element, dense_index is
        // now equal to dense.size(), so this evaluates to end().
        return this->dense.begin() + static_cast<typename std::vector<T>::difference_type>(dense_index);
    }


    [[nodiscard]]
    bool contains(const slot_handle handle) const noexcept
    {
        if (handle.index >= this->slots.size())
            return false;

        const slot_record& slot = this->slots[handle.index];

        return
            slot.dense_index != slot_handle::invalid_index &&
            slot.generation == handle.generation;
    }


    T* get(const slot_handle handle) noexcept
    {
        if (!this->contains(handle))
            return nullptr;

        return &this->dense[this->slots[handle.index].dense_index];
    }


    const T* get(const slot_handle handle) const noexcept
    {
        if (!this->contains(handle))
            return nullptr;

        return &this->dense[this->slots[handle.index].dense_index];
    }


    // Fast convenience access.
    //
    // This assumes the handle is valid.
    T& operator[](const slot_handle handle) noexcept
    {
        return this->dense[this->slots[handle.index].dense_index];
    }


    const T& operator[](const slot_handle handle) const noexcept
    {
        return this->dense[this->slots[handle.index].dense_index];
    }


    // Retrieve the stable handle belonging to an object while
    // iterating by dense index.
    slot_handle handle_at(const size_type dense_index) const noexcept
    {
        const std::uint32_t slot_index = this->dense_to_slot[dense_index];

        return {
            slot_index,
            this->slots[slot_index].generation
        };
    }


    // Invalidates all current handles but keeps allocated memory.
    void clear() noexcept
    {
        this->dense.clear();
        this->dense_to_slot.clear();

        this->free_head = slot_handle::invalid_index;

        for (std::uint32_t i = 0; i < this->slots.size(); ++i)
        {
            slot_record& slot = this->slots[i];

            slot.dense_index = slot_handle::invalid_index;

            slot.generation = this->next_generation(slot.generation);

            slot.next_free = this->free_head;

            this->free_head = i;
        }
    }


    [[nodiscard]]
    size_type size() const noexcept
    {
        return this->dense.size();
    }


    [[nodiscard]]
    bool empty() const noexcept
    {
        return this->dense.empty();
    }


    [[nodiscard]]
    size_type capacity() const noexcept
    {
        return this->dense.capacity();
    }


    T* data() noexcept
    {
        return this->dense.data();
    }


    const T* data() const noexcept
    {
        return this->dense.data();
    }


    iterator begin() noexcept
    {
        return this->dense.begin();
    }


    iterator end() noexcept
    {
        return this->dense.end();
    }


    const_iterator begin() const noexcept
    {
        return this->dense.begin();
    }


    const_iterator end() const noexcept
    {
        return this->dense.end();
    }


    const_iterator cbegin() const noexcept
    {
        return this->dense.cbegin();
    }


    const_iterator cend() const noexcept
    {
        return this->dense.cend();
    }


private:

    // -------------------------------------------------------------------------
    // Performs the actual deletion using a dense index.
    //
    // Both erase(slot_handle) and erase(iterator) ultimately come through
    // this function so the slot bookkeeping only exists in one place.
    // -------------------------------------------------------------------------

    void erase_dense_index(const std::uint32_t erased_dense_index)
    {
        // Determine which stable slot owns the object being erased.
        const std::uint32_t erased_slot_index = this->dense_to_slot[erased_dense_index];

        slot_record& erased_slot = this->slots[erased_slot_index];

        const std::uint32_t last_dense_index = static_cast<std::uint32_t>(this->dense.size() - 1);


        // If this isn't already the final element,
        // move the final element into the hole.
        if (erased_dense_index != last_dense_index)
        {
            this->dense[erased_dense_index] = std::move(this->dense.back());

            // Find the stable slot belonging to the object
            // that was just moved.
            const std::uint32_t moved_slot_index = this->dense_to_slot.back();

            // The moved object now occupies erased_dense_index.
            this->dense_to_slot[erased_dense_index] = moved_slot_index;

            // Update the moved object's slot so its existing
            // slot_handle still resolves correctly.
            this->slots[moved_slot_index].dense_index = erased_dense_index;
        }


        // Remove the now-redundant final dense element.
        this->dense.pop_back();

        this->dense_to_slot.pop_back();

        // Invalidate all existing handles to the erased object.
        erased_slot.dense_index = slot_handle::invalid_index;

        erased_slot.generation = this->next_generation(erased_slot.generation);

        // Add the freed slot to the free list.
        erased_slot.next_free = this->free_head;

        this->free_head = erased_slot_index;
    }


    static constexpr size_type max_elements() noexcept
    {
        return static_cast<size_type>(slot_handle::invalid_index);
    }


    static constexpr std::uint32_t next_generation(std::uint32_t generation) noexcept
    {
        ++generation;

        // Reserve zero for an uninitialized/default handle.
        if (generation == 0)
            ++generation;

        return generation;
    }
};