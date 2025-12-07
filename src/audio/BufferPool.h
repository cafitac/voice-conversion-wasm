/**
 * BufferPool.h
 *
 * 메모리 풀링: 반복 사용되는 버퍼를 재활용하여 할당/해제 오버헤드 감소
 */

#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <vector>
#include <memory>
#include <mutex>

class BufferPool {
public:
    static BufferPool& getInstance() {
        static BufferPool instance;
        return instance;
    }

    /**
     * 버퍼 할당 (풀에서 재사용 또는 새로 생성)
     */
    std::vector<float> acquire(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 적절한 크기의 버퍼 찾기
        for (auto it = pool_.begin(); it != pool_.end(); ++it) {
            if (it->capacity() >= size) {
                std::vector<float> buffer = std::move(*it);
                pool_.erase(it);
                buffer.resize(size);
                return buffer;
            }
        }

        // 풀에 없으면 새로 생성
        std::vector<float> buffer;
        buffer.reserve(size * 1.5); // 여유 공간 확보
        buffer.resize(size);
        return buffer;
    }

    /**
     * 버퍼 반환 (풀에 저장하여 재사용)
     */
    void release(std::vector<float>&& buffer) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 풀 크기 제한 (최대 10개)
        if (pool_.size() < 10) {
            pool_.push_back(std::move(buffer));
        }
    }

    /**
     * 풀 초기화
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.clear();
    }

private:
    BufferPool() = default;
    ~BufferPool() = default;
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    std::vector<std::vector<float>> pool_;
    std::mutex mutex_;
};

#endif // BUFFER_POOL_H
