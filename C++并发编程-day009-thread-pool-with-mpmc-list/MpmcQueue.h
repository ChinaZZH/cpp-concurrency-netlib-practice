
#pragma once
#include <atomic>
#include <vector>
#include <cstddef>


template<typename T>
class MPMC_Queue
{
public:
	explicit MPMC_Queue(size_t capactiy)
		:m_capacity_(nextPowerOf2(capactiy))
		, m_mask_(m_capacity_ - 1)
		, m_vecSlots_(m_capacity_)
		, m_enqueue_pos_(0)
		, m_dequeue_pos_(0)
	{
		for (int i = 0; i < m_capacity_; ++i)
		{
			m_vecSlots_[i].seq_no.store(i, std::memory_order_relaxed);
		}
	}

	bool enqueue(const T& data)
	{
		uint64_t enqueue_pos = m_enqueue_pos_.load(std::memory_order_relaxed);
		while (true)
		{
			uint64_t pos_index = enqueue_pos & m_mask_;
			uint64_t SeqNo = m_vecSlots_[pos_index].seq_no.load(std::memory_order_acquire);
			int64_t nDiffOffset = static_cast<int64_t>(SeqNo) - static_cast<int64_t>(enqueue_pos);
			if (0 == nDiffOffset)
			{
				if (m_enqueue_pos_.compare_exchange_weak(enqueue_pos,
					enqueue_pos + 1,
					std::memory_order_release,
					std::memory_order_relaxed)) {

					m_vecSlots_[pos_index].data = data;
					m_vecSlots_[pos_index].seq_no.store(enqueue_pos + 1, std::memory_order_release);
					return true;
				}
			}
			else if (nDiffOffset < 0)
			{
				// 队列已经满了
				return false;
			}
			else
			{
				// 其他生产队列已经先生产了，则重新获取最新的写入结点
				enqueue_pos = m_enqueue_pos_.load(std::memory_order_relaxed);
			}
		}

		return false;
	}

	bool dequeue(T& data)
	{
		uint64_t dequeue_pos = m_dequeue_pos_.load(std::memory_order_relaxed);
		while (1)
		{
			uint64_t pos_index = dequeue_pos & m_mask_;
			uint64_t SeqNo = m_vecSlots_[pos_index].seq_no.load(std::memory_order_acquire);
			int64_t nDiffOffset = static_cast<int64_t>(SeqNo) - static_cast<int64_t>(dequeue_pos + 1);
			if (0 == nDiffOffset)
			{
				if (m_dequeue_pos_.compare_exchange_weak(dequeue_pos,
					dequeue_pos + 1,
					std::memory_order_release,
					std::memory_order_relaxed)) {

					data = m_vecSlots_[pos_index].data;
					m_vecSlots_[pos_index].seq_no.store(dequeue_pos + m_capacity_, std::memory_order_release);
					return true;
				}
			}
			else if (nDiffOffset < 0)
			{
				// 队列已经空了
				return false;
			}
			else
			{
				dequeue_pos = m_dequeue_pos_.load(std::memory_order_relaxed);
			}
		}

		return false;
	}

private:
	static size_t nextPowerOf2(size_t n)
	{
		size_t cap = 1;
		while (cap < n) {
			cap = cap << 1;
		}

		return cap;
	}


	struct Slot
	{
		T data;
		std::atomic<uint64_t>  seq_no;

		Slot() = default;
	};

	const size_t m_capacity_;
	const size_t m_mask_;
	std::vector<Slot> m_vecSlots_;
	std::atomic<uint64_t> m_enqueue_pos_;
	std::atomic<uint64_t> m_dequeue_pos_;
};



