# Real-Time Stock Trading Engine

A lock-free, high-concurrency stock trading engine that matches buy and sell orders in real-time, designed to meet Onymos's technical specifications.

## Overview
This solution implements a real-time order matching engine for 1,024 stock tickers. It uses lock-free data structures to handle concurrent order additions and matches buy/sell orders in **O(n)** time complexity. The system simulates real-world stock transactions with a multithreaded wrapper to stress-test concurrency and correctness.

---

## Key Features
1. **Supports 1,024 Tickers**  
   Uses a fixed-size array with hash-based indexing to map ticker symbols to order books without dictionaries/maps.

2. **Lock-Free Order Books**  
   Buy/sell orders are stored in sorted linked lists using atomic Compare-And-Swap (CAS) operations for thread-safe insertions and deletions.

3. **O(n) Matching Algorithm**  
   Matches buy orders with sell orders by iterating through order lists once, ensuring efficient price-time priority.

4. **Concurrency-Ready**  
   Handles race conditions via atomic operations and lock-free linked lists, simulating real-world multithreaded stockbroker activity.

---

## Technical Approach
### 1. Handling 1,024 Tickers
- **Hash-Based Indexing**: Ticker symbols (e.g., `AAPL`) are hashed to an array index (`hash(ticker) % 1024`).  
- **Trade-off**: Hash collisions are possible but minimized by the fixed-size constraint.

### 2. Lock-Free Order Books
- **Data Structures**:  
  - Each `OrderBook` contains two sorted linked lists:  
    - **Buy Orders**: Sorted in descending order (highest bid first).  
    - **Sell Orders**: Sorted in ascending order (lowest ask first).  
- **Insertion Logic**:  
  - Orders are added using CAS to ensure thread safety without locks.  
  - Example: A sell order for $150 is placed after traversing the list until finding a price ≥ $150.

### 3. Order Matching
- **Algorithm**:  
  1. Traverse buy and sell lists.  
  2. Match orders where `buy.price ≥ sell.price`.  
  3. Update quantities and remove filled orders atomically.  
- **Complexity**: O(n) per `matchOrder` call, where `n` is the number of orders.

### 4. Concurrency
- **Atomic Operations**: `std::atomic` (C++) ensures thread-safe modifications.  
- **ABA Prevention**: Nodes are marked as logically deleted before removal.  

---

## Code Structure
- **`Order` Struct**: Stores order type (buy/sell), quantity, price, and atomic pointer to the next order.  
- **`OrderBook` Struct**: Contains atomic pointers to buy/sell order lists.  
- **`addOrder` Function**:  
  - Inserts orders into sorted lists using CAS.  
  - Handles hash collisions via ticker-to-index mapping.  
- **`matchOrder` Function**:  
  - Merges compatible buy/sell orders and updates quantities.  
- **`simulateOrders` Function**:  
  - Generates 10,000 random orders across 10 threads to test concurrency.  

---

## Simulation
To simulate real-world trading:
```cpp
void simulateOrders() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) { // 10 threads
        threads.emplace_back([](){
            for (int j = 0; j < 1000; ++j) { // 1000 orders/thread
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
