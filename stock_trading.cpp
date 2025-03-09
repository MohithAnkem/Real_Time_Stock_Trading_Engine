#include <atomic>
#include <vector>
#include <string>
#include <cstdlib>
#include <functional>
#include <thread>

struct Order {
    bool isBuy;
    int quantity;
    int price;
    std::atomic<Order*> next;
    Order(bool isBuy, int qty, int price) : isBuy(isBuy), quantity(qty), price(price), next(nullptr) {}
};

struct OrderBook {
    std::atomic<Order*> buyHead;
    std::atomic<Order*> sellHead;
    OrderBook() : buyHead(nullptr), sellHead(nullptr) {}
};

const int NUM_TICKERS = 1024;
std::vector<OrderBook> orderBooks(NUM_TICKERS);

void addOrder(bool isBuy, const std::string& ticker, int qty, int price) {
    // Hash ticker to index
    size_t hash = std::hash<std::string>{}(ticker);
    int index = hash % NUM_TICKERS;
    OrderBook& book = orderBooks[index];
    
    Order* newOrder = new Order(isBuy, qty, price);
    std::atomic<Order*>& head = isBuy ? book.buyHead : book.sellHead;

    // Lock-free insertion into sorted list
    Order* current = head.load();
    Order* prev = nullptr;
    while (true) {
        // Find insertion point
        while (current && ((isBuy && current->price > price) || (!isBuy && current->price < price))) {
            prev = current;
            current = current->next.load();
        }
        newOrder->next = current;
        if (prev) {
            if (prev->next.compare_exchange_weak(current, newOrder)) break;
        } else {
            if (head.compare_exchange_weak(current, newOrder)) break;
        }
        // Retry on conflict
        current = head.load();
        prev = nullptr;
    }
}

void matchOrder() {
    for (auto& book : orderBooks) {
        Order* buy = book.buyHead.load();
        Order* sell = book.sellHead.load();
        while (buy && sell) {
            if (buy->price >= sell->price) {
                int matchQty = std::min(buy->quantity, sell->quantity);
                // Update quantities
                buy->quantity -= matchQty;
                sell->quantity -= matchQty;
                // Remove filled orders
                if (buy->quantity == 0) {
                    book.buyHead.compare_exchange_strong(buy, buy->next.load());
                    delete buy;
                }
                if (sell->quantity == 0) {
                    book.sellHead.compare_exchange_strong(sell, sell->next.load());
                    delete sell;
                }
                buy = book.buyHead.load();
                sell = book.sellHead.load();
            } else break;
        }
    }
}

// Wrapper to simulate transactions
void simulateOrders() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) { // 10 threads
        threads.emplace_back([](){
            for (int j = 0; j < 1000; ++j) { // 1000 orders per thread
                bool isBuy = rand() % 2;
                std::string ticker = "TICKER_" + std::to_string(rand() % NUM_TICKERS);
                int qty = rand() % 100 + 1;
                int price = rand() % 1000 + 1;
                addOrder(isBuy, ticker, qty, price);
                matchOrder();
            }
        });
    }
    for (auto& t : threads) t.join();
}

int main() {
    simulateOrders();
    return 0;
}