#include <iostream>
#include <memory>

int main() {
    {
        std::shared_ptr<void> p(nullptr, [](void*) { std::cout << "DELETER CALLED\n"; });
    }
    std::cout << "DONE\n";
    return 0;
}
