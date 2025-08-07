# 💳 ESP32 Payment Gateway System (ESP-IDF + WiFi/LAN + Midtrans)

A secure, embedded payment system built with **ESP32** using the **ESP-IDF framework** with **WiFi and LAN connectivity** for cloud-based digital payments via [Midtrans](https://midtrans.com).

Designed for offline-capable smart systems such as:
- Electric massage chair

---

## ⚙️ Tech Stack

- 🔧 **ESP-IDF** (>= v5.0)
- 📶 **WiFi** for wireless internet connectivity
- 🔌 **LAN/Ethernet** via external module for wired connectivity
- 💳 **Midtrans Core/Snap API** (HTTP POST)
- 📟 Display: ILI9341+XPT2046 touchscreen   
- 📂 Optional MQTT/HTTP logging integration

---

## 🧰 Hardware Requirements

| Module       | Description                                      |
|--------------|--------------------------------------------------|
| ESP32        | WROOM/WROVER or compatible dev board             |
| LAN Module   | External Ethernet/LAN module (optional)          |
| Display+Touch| ILI9341+XPT2046 touchscreen interface           |
| Relay Module | For output trigger (after successful payment)    |
| Power        | Stable 5V 2A supply for reliable operation       |

---

## 🧠 Features

� WiFi connectivity with automatic reconnection
🔌 LAN/Ethernet support for wired connectivity  
🔐 Secure Midtrans Payment Integration
💵 Bill Acceptor support for cash payments
📤 QRIS URL generation and delivery
⏳ Auto-timeout & retry if no payment
🧾 Transaction status polling
💡 Output trigger on settlement
📁 Logs to NVS or optional server

---

## 📝 Development History & Improvements

### 🔧 Memory & Performance Optimizations
- **DEBUG_MODE System Implementation** - Conditional compilation system with zero overhead in production builds
- **Stack Overflow Prevention** - Increased stack sizes for critical tasks and added monitoring
- **Memory Leak Prevention** - Proper memory cleanup in transaction tasks and UI components
- **Buffer Optimization** - Static buffer allocation to reduce stack usage in network tasks

### 🌐 Network & Connectivity Enhancements  
- **Dual Network Support** - WiFi and LAN connectivity with runtime switching
- **Internet Connectivity Testing** - Real internet connectivity validation beyond WiFi connection status
- **HTTP Request Optimization** - Improved error handling and connection management
- **Network Status Indicators** - Real-time WiFi signal indicators with blinking alerts

### 🖥️ User Interface Improvements
- **Payment Task Management** - Prevention of double-clicks and concurrent payment tasks
- **UI State Management** - Proper button enable/disable states during payment processing
- **Screen Transition Safety** - Safe page navigation with memory cleanup
- **Touch Response Optimization** - Improved touch responsiveness and feedback

### 🔒 Security & Stability Features
- **Buffer Overflow Protection** - Added bounds checking for all string operations
- **Device Info Validation** - Corruption detection and safe defaults for device parameters
- **Transaction Safety** - Atomic transaction operations with rollback capabilities
- **Error Recovery** - Graceful error handling with automatic UI restoration

### 💾 Data Management & Storage
- **NVS Parameter Validation** - Safe loading/saving of configuration parameters
- **Transaction Logging** - Comprehensive transaction history with error tracking
- **Configuration Management** - Runtime configuration updates without restart
- **Data Integrity Checks** - Validation of critical system parameters

### ⚡ System Performance
- **Task Priority Optimization** - Balanced task priorities for smooth operation
- **Watchdog Integration** - System stability monitoring and auto-recovery
- **Resource Management** - Efficient memory and CPU usage optimization
- **Background Processing** - Non-blocking operations for better user experience

### 🐛 Bug Fixes & Stability
- **Printf Memory Usage** - Replaced all printf statements with DEBUG_MODE system
- **Stack Overflow Fixes** - Resolved task stack overflow issues in payment processing
- **Memory Corruption Prevention** - Fixed buffer overflows and pointer validation
- **UI Threading Issues** - Resolved race conditions in UI updates
- **Network Timeout Handling** - Improved timeout and retry mechanisms

### 📊 Monitoring & Debugging
- **Stack Usage Monitoring** - Real-time stack usage reporting for critical tasks
- **Network Status Logging** - Comprehensive network connectivity debugging
- **Payment Flow Tracking** - Detailed payment process monitoring
- **Error Classification** - Structured error reporting and classification

---



