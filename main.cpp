#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

enum class CollectionAction { ADD, REMOVE, CLEAR, ASSIGN };

std::string to_string(CollectionAction action) {
    switch(action) {
      case CollectionAction::ADD:
        return "add";
      case CollectionAction::REMOVE:
        return "remove";
      case CollectionAction::CLEAR:
        return "clear";
      case CollectionAction::ASSIGN:
        return "assign";
    }
    return "";
}

struct CollectionChangeNotification {
    CollectionAction action;
    std::vector<size_t> item_indexes;
};

class CollectionObserver {
  public:
    virtual void CollectionChanged(CollectionChangeNotification notification) = 0;
    virtual ~CollectionObserver() = default;
};

template <typename T, class Allocator = std::allocator<T>>
class ObservableVector final {
    using size_type = typename std::vector<T, Allocator>::size_type;
  public:
    explicit ObservableVector(const Allocator& alloc) noexcept : data_(alloc) {}
    ObservableVector() noexcept : ObservableVector(Allocator()) {}

    explicit ObservableVector(size_type count, const Allocator& alloc = Allocator()) : data_(count, alloc) {}
    ObservableVector(size_type count, const T& value, const Allocator& alloc = Allocator()) : data_(count, value, alloc) {}

    ObservableVector(ObservableVector&& other) noexcept : data_(other.data) {}
    ObservableVector(ObservableVector&& other, const Allocator& alloc) : data_(other.data, alloc) {}

    ObservableVector(std::initializer_list<T> init, const Allocator& alloc = Allocator()) : data_(init, alloc) {}

    template <class InputIt>
    ObservableVector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) : data_(first, last, alloc) {}

    ObservableVector& operator=(const ObservableVector& other) {
        if (this != &other) {
            data_ = other.data_;
            for (auto* observer : observers) {
                if (observer) {
                    observer->CollectionChanged({CollectionAction::ASSIGN, std::vector<size_t>{}});
                }
            }
        }
        return *this;
    }

    ObservableVector& operator=(ObservableVector&& other) {
        if (this != &other) {
            data_ = std::move(other.data_);
            for (auto* observer : observers) {
                if (observer) {
                    observer->CollectionChanged({CollectionAction::ASSIGN, std::vector<size_t>{}});
                }
            }
        }
        return *this;
    }

    void PushBack(T&& value) {
        data_.push_back(value);
        for (auto* observer : observers) {
            if (observer) {
                observer->CollectionChanged({CollectionAction::ADD, std::vector<size_t>{data_.size() - 1}});
            }
        }
    }

    void PopBack() {
        data_.pop_back();
        for (auto* observer : observers) {
            if (observer) {
                observer->CollectionChanged({CollectionAction::REMOVE, std::vector<size_t>{data_.size()}});
            }
        }
    }

    void Clear() noexcept {
        data_.clear();
        for (auto* observer : observers) {
            if (observer) {
                observer->CollectionChanged({CollectionAction::CLEAR, std::vector<size_t>{}});
            }
        }
    }

    [[nodiscard]] size_type size() const noexcept {
        return data_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return data_.empty();
    }

    void AddObserver(CollectionObserver* const o) {
        observers.push_back(o);
    }

    void RemoveObserver(const CollectionObserver* const o) {
        observers.erase(std::remove(std::begin(observers), std::end(observers), o), std::end(observers));
   }

  private:
    std::vector<T, Allocator> data_;
    std::vector<CollectionObserver*> observers;
};

class Observer : public CollectionObserver {
  public:
    void CollectionChanged(CollectionChangeNotification notification) override {
        std::cout << "action: " << to_string(notification.action);
        if (!notification.item_indexes.empty()) {
            std::cout << ", indexes: ";
            for (auto i : notification.item_indexes) {
                std::cout << i << ' ';
            }
        }
        std::cout << std::endl;
    }
};

int main() {
    ObservableVector<int> v;
    Observer o;

    v.AddObserver(&o);

    v.PushBack(1);
    v.PushBack(2);

    v.PopBack();

    v.Clear();

    v.RemoveObserver(&o);

    v.PushBack(3);
    v.PushBack(4);

    v.AddObserver(&o);

    ObservableVector<int> v2 {1, 2, 3};
    v = v2;

    v = ObservableVector<int>{7, 8, 9};
}
