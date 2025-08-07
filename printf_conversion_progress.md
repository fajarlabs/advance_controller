# printf to DEBUG Conversion Template

## Files to Co5. ✅ ui_Screen2.c (Payment screen) - FULLY CONVERTEDvert:

### 1. usr_k2_lan.c - UART Communication
- ✅ Include added
- ⚠️ Partially converted (need to continue)

### 2. my_wifi.c - WiFi Communication  
- ✅ Include added
- ✅ All printf converted to DEBUG_PRINTLN

### 3. main.c - Main Application
- ✅ Include added
- ✅ All printf converted to DEBUG_PRINTLN

### 4. ui_Screen1.c - Initial Splash Screen
- ✅ Include added
- ✅ All printf converted to DEBUG_PRINTLN

### 5. ui_Screen4.c - Error Screen
- ✅ Include already added
- ✅ All printf converted to DEBUG_PRINTLN

### 6. ui_Screen6.c - Payment Success Screen
- ✅ Include already added
- ✅ All printf converted to DEBUG_PRINTLN

### 7. ui_Screen7.c - Statistics Screen
- ✅ Include added
- ✅ All printf converted to DEBUG_PRINTLN

### 8. ui_Screen8.c - Payment Screen
- ⏳ Need to add include
- ⏳ Need to convert printf

## Conversion Pattern:

```c
// Before:
printf("Message\n");
printf("Message with param: %s\n", param);
printf("Message without newline");

// After:
DEBUG_PRINTLN("Message");
DEBUG_PRINTLN("Message with param: %s", param);
DEBUG_PRINT("Message without newline");
```

## Files to Add Include:

Add this line to all .c files:
```c
#include "my_global_lib.h"
```

## Priority Order:
1. ✅ main.c (Core application) - COMPLETED
2. ✅ nvs_helper.c (NVS data loading) - COMPLETED
3. ✅ my_wifi.c (WiFi - Critical for connectivity) - COMPLETED
4. ✅ ui_Screen1.c (Initial splash screen) - COMPLETED
5. ✅ ui_Screen2.c (Payment screen) - FULLY CONVERTED
6. ✅ ui_Screen4.c (Error screen) - COMPLETED
7. ✅ ui_Screen6.c (Payment success screen) - COMPLETED
8. ✅ ui_Screen7.c (Statistics screen) - COMPLETED
9. ⚠️ usr_k2_lan.c (UART - Critical for LAN) - PARTIALLY DONE
10. ⏳ ui_Screen8.c (Payment processing)

## Production Setup:
Set `DEBUG_MODE 0` in my_global_lib.h for production builds.
