#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>

#define MAX_FIELDS     10     // Maksimal jumlah field
#define MAX_FIELD_LEN  290    // Panjang maksimal tiap field
#define MAX_RETRY      100    // Agar tidak infinite loop

// Fungsi-fungsi eksekusi berdasarkan tipe transaksi
void execute_info(char fields[MAX_FIELDS][MAX_FIELD_LEN]);
void execute_ordercheck(char fields[MAX_FIELDS][MAX_FIELD_LEN]);
void execute_payment(char fields[MAX_FIELDS][MAX_FIELD_LEN]);
void execute_addtransaction(char fields[MAX_FIELDS][MAX_FIELD_LEN]);
void execute_else(void);

// Cek apakah semua karakter dalam data adalah ASCII
bool is_all_ascii(const char *data);

#endif // TRANSACTION_H
