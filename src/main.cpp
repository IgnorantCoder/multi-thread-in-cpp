#include <cassert>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <numeric>
#include <algorithm>

class MultiThreadPrinter {
public:
    template <typename T0, typename T1>
    void print(const T0& t0, const T1& t1) const 
    {
        std::lock_guard<std::mutex> lock(this->getMutex());
        std::cout << t0 << t1 << std::endl;
    }

private:
    std::mutex& getMutex() const 
    {
        static std::mutex mtx;
        return mtx;
    }
};

template <std::size_t N>
class MultiThreadBinary {
public:
    template <typename T>
    std::vector<T> add(
        const std::vector<T>& t0,
        const std::vector<T>& t1) const
    {
        return this->calulate(t0, t1, [](const T& x0, const T& x1) { return x0 + x1; });
    }

    template <typename T>
    std::vector<T> sub(
        const std::vector<T>& t0,
        const std::vector<T>& t1) const
    {
        return this->calulate(t0, t1, [](const T& x0, const T& x1) { return x0 - x1; });
    }

    template <typename T>
    std::vector<T> mult(
        const std::vector<T>& t0,
        const std::vector<T>& t1) const
    {
        return this->calulate(t0, t1, [](const T& x0, const T& x1) { return x0 * x1; });
    }

    template <typename T>
    std::vector<T> div(
        const std::vector<T>& t0,
        const std::vector<T>& t1) const
    {
        return this->calulate(t0, t1, [](const T& x0, const T& x1) { return x0 / x1; });
    }

private:
    template <typename T, typename F>
    std::vector<T> calulate(
        const std::vector<T>& t0,
        const std::vector<T>& t1,
        const F& binary) const
    {
        assert(t0.size() == t1.size());

        std::vector<std::size_t> threadIds(N);
        std::iota(threadIds.begin(), threadIds.end(), 0);

        std::vector<std::size_t> index(t0.size());
        std::iota(index.begin(), index.end(), 0);

        std::list<std::vector<std::size_t>> targets;
        std::transform( // packing calculation group
            threadIds.cbegin(),
            threadIds.cend(),
            std::back_inserter(targets),
            [&index](const auto id) {
            std::vector<std::size_t> target;
            std::copy_if(
                index.cbegin(),
                index.cend(),
                std::back_inserter(target),
                [id](const std::size_t i) { return i % N == id; });
            return target;
        });


        std::list<std::thread> threads;
        std::vector<T> ret(t0.size());
        auto&& add = [&ret, &t0, &t1, &binary](const auto i) {
            ret[i] = binary(t0[i], t1[i]);
        };
        const auto& exec = [&add](const auto& target) {
            std::for_each(target.begin(), target.end(), std::move(add));
        };

        std::transform( // execute calculation.
            targets.cbegin(),
            targets.cend(),
            std::back_inserter(threads),
            [&exec, &threads](const auto& target) {
            return std::thread(exec, target);
        });

        std::for_each(  // wait calculation.
            threads.begin(),
            threads.end(),
            [](auto& thread) { thread.join(); });

        return ret;
    }
};


int main() {
    {
        const MultiThreadPrinter mt;
        std::thread th0([&mt]() { mt.print("id:", std::this_thread::get_id()); });
        std::thread th1([&mt]() { mt.print("id:", std::this_thread::get_id()); });
        std::thread th2([&mt]() { mt.print("id:", std::this_thread::get_id()); });
        th0.join();
        th1.join();
        th2.join();
    }

    {
        const MultiThreadBinary<2> mt;
        const std::vector<float> v0({ 1, 9, 8, 6, 7, 2, 2 });
        const std::vector<float> v1({ 4, 1, 2, 1, 6, 8, 9 });
        for (const auto e : mt.add(v0, v1)) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;

        for (const auto e : mt.sub(v0, v1)) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;

        for (const auto e : mt.mult(v0, v1)) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;

        for (const auto e : mt.div(v0, v1)) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;

    }

    std::system("PAUSE");
    return 0;
}
