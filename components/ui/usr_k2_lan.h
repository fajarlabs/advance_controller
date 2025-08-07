#ifndef USR_K2_LAN_H
#define USR_K2_LAN_H

#ifdef __cplusplus
extern "C"
{
#endif

    void usr_k2_init(void);
    void init_uart(void);
    bool send_at_command(const char *msg);
    void uart_read_response(void *arg);
    void parse_uart_response(const char *input);

#ifdef __cplusplus
}
#endif

#endif // SIM7000E_H