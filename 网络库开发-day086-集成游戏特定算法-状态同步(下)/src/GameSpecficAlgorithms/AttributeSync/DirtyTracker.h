#pragma once
#include <cstdint>
#include <stdexcept>

// 这个是单线程版本的，如果有需要之后再修改扩展多线程，目前先这样。
class DirtyTracker {
public:
    DirtyTracker()
    :dirtyBits_(0)
    { }

    ~DirtyTracker() = default;      

public:
    void MarkDirty(int attributeIndex) {
        if (attributeIndex < 0 || attributeIndex >= 64) {
            throw std::out_of_range("Attribute index out of range");
        }
        dirtyBits_ |= (1ULL << attributeIndex);
    }

    void ClearDirty(int attributeIndex) {
        if (attributeIndex < 0 || attributeIndex >= 64) {
            throw std::runtime_error("Attribute index out of range");
        }
        dirtyBits_ &= ~(1ULL << attributeIndex);
    }

    bool IsDirty(int attributeIndex) const {
        if (attributeIndex < 0 || attributeIndex >= 64) {
            throw std::runtime_error("Attribute index out of range");
        }
        return (dirtyBits_ & (1ULL << attributeIndex)) != 0;
    }

    void ClearAll() {
        dirtyBits_ = 0;
    }

    uint64_t GetDirtyBits() const {
        return dirtyBits_;
    }

    bool HasAnyDirty() const {
        return dirtyBits_ != 0;
    }

    /*
    bool IsBeFallbackToFull() const {
        // 如果脏位数超过32个，则认为需要回退到全量同步
        return CountDirtyBits() > 32;
    }
    

private:
    int CountDirtyBits() const {
        int count = 0;
        uint64_t bits = dirtyBits_;
        while (bits) {
            count += bits & 1;
            bits >>= 1;
        }
        
        return count;
    }
*/

private:
    uint64_t dirtyBits_ = 0; // 64位脏位标记，最多支持64个属性
};