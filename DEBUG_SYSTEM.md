# Debug System Documentation

## Overview
Sistem debug yang fleksibel untuk mengontrol output printf dalam aplikasi ESP32 payment system.

## Konfigurasi

### 1. Compile-time Configuration
Di file `my_global_lib.h`:
```c
#define DEBUG_MODE 1  // Set to 1 untuk enable, 0 untuk disable
```

### 2. Runtime Configuration
Anda bisa mengubah status debug saat runtime:
```c
set_debug_mode(true);   // Enable debug
set_debug_mode(false);  // Disable debug
bool status = get_debug_mode();  // Check current status
```

## Penggunaan Makro Debug

### 1. DEBUG_PRINT (tanpa newline)
```c
DEBUG_PRINT("Payment amount: %s", amount);
```

### 2. DEBUG_PRINTLN (dengan newline)
```c
DEBUG_PRINTLN("Payment button clicked");
DEBUG_PRINTLN("Status: %s", status);
```

### 3. Runtime Debug Functions
```c
debug_printf("Runtime debug: %d", value);
debug_println("Runtime debug with newline");
```

## Cara Mengganti printf Existing

### Sebelum:
```c
printf("Button enabled for payment\n");
printf("Payment amount: %s\n", amount);
```

### Sesudah:
```c
DEBUG_PRINTLN("Button enabled for payment");
DEBUG_PRINTLN("Payment amount: %s", amount);
```

## Keuntungan

1. **Memory Efficiency**: Debug prints tidak menggunakan memory di production build
2. **Performance**: Zero overhead saat DEBUG_MODE = 0
3. **Flexibility**: Bisa enable/disable saat runtime
4. **Compatibility**: Drop-in replacement untuk printf

## Production Build

Untuk production build, set:
```c
#define DEBUG_MODE 0
```

Semua debug prints akan di-compile out dan tidak menggunakan memory atau CPU cycles.

## Best Practices

1. Gunakan `DEBUG_PRINTLN` untuk logging umum
2. Gunakan `DEBUG_PRINT` untuk output yang perlu formatting khusus
3. Gunakan runtime functions untuk debug yang bisa diubah saat aplikasi berjalan
4. Selalu test dengan DEBUG_MODE=0 sebelum production deployment

## Memory Impact

- **DEBUG_MODE=1**: Normal printf memory usage
- **DEBUG_MODE=0**: Zero memory usage (compiled out)
- Runtime functions: Minimal overhead untuk check boolean flag

## Timer Callback Optimization

Untuk callback yang sering dipanggil (seperti timer), gunakan conditional logging:
```c
static int log_counter = 0;
if (++log_counter % 10 == 0) {
    DEBUG_PRINTLN("Timer callback executed 10 times");
}
```
